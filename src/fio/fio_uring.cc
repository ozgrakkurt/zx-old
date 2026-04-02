#include <fio/fio_uring.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>
#include <zx/zx_ptr_cast.h>
#include <zx/zx_sys.h>
#include <zx/zx_sys_io_uring.h>

ZXStatus FioUring::init(FioUringParams const *params, FioUring *fio_ring,
                        ZXErrStack *err_stack) {
  ZX_ASSERT((fio_ring != nullptr));
  ZX_ASSERT((err_stack != nullptr));

  zx_u32 sqes_sz = 0;
  zx_u32 ring_sz = 0;
  zx_u32 sring_sz = 0;
  zx_u32 cring_sz = 0;

  zx_u32 const *sq_khead = nullptr;
  zx_u32 *sq_ktail = nullptr;
  zx_u32 *cq_khead = nullptr;
  zx_u32 const *cq_ktail = nullptr;

  zx_u8 *ring = nullptr;
  zx_u8 *sqes = nullptr;
  zx_i32 ring_fd = -1;
  bool registered_napi = false;
  bool registered_buffer = false;
  ZXStatus error_status = ZXStatus::Ok;

  zx_u32 flags = IORING_SETUP_SQPOLL | IORING_SETUP_SINGLE_ISSUER |
                 IORING_SETUP_NO_SQARRAY;

  if (params->polled_io) {
    flags |= IORING_SETUP_IOPOLL;
  }

  if (params->wq_fd > 0) {
    flags |= IORING_SETUP_ATTACH_WQ;
  }

  io_uring_params ring_params;
  __builtin_memset(&ring_params, 0, sizeof(io_uring_params));

  ring_params.wq_fd = (zx_u32)params->wq_fd;
  ring_params.sq_thread_idle = 1000;
  ring_params.flags = flags;

  zx_u32 const n_entries = 128;

  ZX_ERR_TRY(
      (zx_sys_io_uring_setup(n_entries, &ring_params, &ring_fd, err_stack)),
      err_stack);
  ZX_ASSERT((ring_fd >= 0));

  if ((ring_params.features & IORING_FEAT_SINGLE_MMAP) == 0) {
    error_status = ZXStatus::AssertFail;
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  sring_sz =
      zx_chk_add_u32(ring_params.sq_off.array,
                     zx_chk_mul_u32(ring_params.sq_entries, sizeof(zx_u32)));

  // sanity check. it should always be 16
  static_assert(sizeof(FioCQE) == 16);

  cring_sz =
      zx_chk_add_u32(ring_params.cq_off.cqes,
                     zx_chk_mul_u32(ring_params.cq_entries, sizeof(FioCQE)));

  ring_sz = zx_int_max_u32(sring_sz, cring_sz);

  error_status = zx_sys_mmap(0, ring_sz, ZX_SYS_PROT_READ | ZX_SYS_PROT_WRITE,
                             ZX_SYS_MAP_SHARED | ZX_SYS_MAP_POPULATE, ring_fd,
                             IORING_OFF_SQ_RING, &ring, err_stack);
  if (error_status != ZXStatus::Ok) {
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  // sanity check. it should always be 64
  static_assert(sizeof(FioSQE) == 64);

  sqes_sz = zx_chk_mul_u32(ring_params.sq_entries, sizeof(FioSQE));
  error_status = zx_sys_mmap(0, sqes_sz, ZX_SYS_PROT_READ | ZX_SYS_PROT_WRITE,
                             ZX_SYS_MAP_SHARED | ZX_SYS_MAP_POPULATE, ring_fd,
                             IORING_OFF_SQES, &sqes, err_stack);
  if (error_status != ZXStatus::Ok) {
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  // check features
  if ((ring_params.features & IORING_FEAT_NODROP) == 0) {
    error_status = ZXStatus::AssertFail;
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }
  if ((ring_params.features & IORING_FEAT_SQPOLL_NONFIXED) == 0) {
    error_status = ZXStatus::AssertFail;
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }
  if ((ring_params.features & IORING_FEAT_RSRC_TAGS) == 0) {
    error_status = ZXStatus::AssertFail;
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  if (!params->polled_io) {
    io_uring_napi napi_params;
    __builtin_memset(&napi_params, 0, sizeof(napi_params));
    napi_params.busy_poll_to = 100000;
    napi_params.prefer_busy_poll = 1;

    error_status = zx_sys_io_uring_register(ring_fd, IORING_REGISTER_NAPI,
                                            &napi_params, 1, err_stack);
    if (error_status != ZXStatus::Ok) {
      ZX_ERR_APPEND_LOC(err_stack);
      goto err;
    }
    registered_napi = true;
  }

  if (params->io_buf == nullptr) {
    ZX_ERR_ASSERT((params->io_buf_size == 0), err_stack);
  } else {
    ZXSysIoVec const io_vec = ZXSysIoVec{
        .iov_base = params->io_buf,
        .iov_len = params->io_buf_size,
    };
    error_status = zx_sys_io_uring_register(ring_fd, IORING_REGISTER_BUFFERS,
                                            &io_vec, 1, err_stack);
    if (error_status != ZXStatus::Ok) {
      ZX_ERR_APPEND_LOC(err_stack);
      goto err;
    }
    registered_buffer = true;
  }

  sq_khead = zx_ptr_cast_mut<zx_u32>(&ring[ring_params.sq_off.head]);
  sq_ktail = zx_ptr_cast_mut<zx_u32>(&ring[ring_params.sq_off.tail]);
  cq_khead = zx_ptr_cast_mut<zx_u32>(&ring[ring_params.cq_off.head]);
  cq_ktail = zx_ptr_cast_mut<zx_u32>(&ring[ring_params.cq_off.tail]);

  ZX_ASSERT(ring_fd > 0);

  // setup output
  *fio_ring = FioUring{
      .sq_kflags = zx_ptr_cast_const<zx_u32>(&ring[ring_params.sq_off.flags]),
      .sq_khead = sq_khead,
      .sq_ktail = sq_ktail,
      .cq_khead = cq_khead,
      .cq_ktail = cq_ktail,
      .sq_head = *sq_khead,
      .sq_tail = *sq_ktail,
      .cq_head = *cq_khead,
      .cq_tail = *cq_ktail,
      .ring_alloc = ring,
      .sqes_alloc = sqes,
      .cqes = zx_ptr_cast_const<FioCQE>(&ring[ring_params.cq_off.cqes]),
      .sqes = zx_ptr_cast_mut<FioSQE>(&ring[ring_params.sq_off.ring_entries]),
      .ring_alloc_size = ring_sz,
      .sqes_alloc_size = sqes_sz,
      .ring_fd = (zx_u32)ring_fd,
      .sq_mask =
          *zx_ptr_cast_const<zx_u32>(&ring[ring_params.sq_off.ring_mask]),
      .sq_n_entries = ring_params.sq_entries,
      .cq_mask =
          *zx_ptr_cast_const<zx_u32>(&ring[ring_params.cq_off.ring_mask]),
      .wq_fd_ = (params->wq_fd > 0) ? (zx_u32)ring_fd : params->wq_fd,
      .registered_buffer = registered_buffer,
      .polled_io = params->polled_io,
  };

  return ZXStatus::Ok;
err:
  if (registered_buffer) {
    ZX_ASSERT((ring_fd > 0));
    ZX_ERR_TRY_FATAL(
        (zx_sys_io_uring_register(ring_fd, IORING_UNREGISTER_BUFFERS, nullptr,
                                  0, err_stack)),
        err_stack);
  }

  if (registered_napi) {
    ZX_ASSERT((ring_fd > 0));
    io_uring_napi napi_params;
    __builtin_memset(&napi_params, 0, sizeof(napi_params));
    ZX_ERR_TRY_FATAL((zx_sys_io_uring_register(ring_fd, IORING_UNREGISTER_NAPI,
                                               &napi_params, 1, err_stack)),
                     err_stack);
  }

  if (sqes != nullptr) {
    ZX_ERR_TRY_FATAL((zx_sys_munmap(sqes, sqes_sz, err_stack)), err_stack);
  }
  if (ring != nullptr) {
    ZX_ERR_TRY_FATAL((zx_sys_munmap(ring, ring_sz, err_stack)), err_stack);
  }
  if (ring_fd > 0) {
    ZX_ERR_TRY_FATAL((zx_sys_close(ring_fd, err_stack)), err_stack);
  }

  ZX_ASSERT((error_status != ZXStatus::Ok));
  ZX_ERR_TRY(error_status, err_stack);

  ZX_ASSERT((false));
  return ZXStatus::Ok;
}

ZXStatus FioUring::deinit(ZXErrStack *err_stack) {
  ZX_ASSERT((this->ring_fd > 0));

  if (this->registered_buffer) {
    ZX_ERR_TRY((zx_sys_io_uring_register((zx_i32)this->ring_fd,
                                         IORING_UNREGISTER_BUFFERS, nullptr, 0,
                                         err_stack)),
               err_stack);
  }

  if (!this->polled_io) {
    io_uring_napi napi_params;
    __builtin_memset(&napi_params, 0, sizeof(napi_params));
    ZX_ERR_TRY(
        (zx_sys_io_uring_register((zx_i32)this->ring_fd, IORING_UNREGISTER_NAPI,
                                  &napi_params, 1, err_stack)),
        err_stack);
  }

  zx_u8 *const ring_alloc_ = this->ring_alloc;
  zx_u32 const ring_sz = this->ring_alloc_size;
  zx_u8 *const sqes_alloc_ = this->sqes_alloc;
  zx_u32 const sqes_sz = this->sqes_alloc_size;

  zx_u32 const ring_fd_ = this->ring_fd;

  ZX_ASSERT((ring_alloc_ != nullptr));
  ZX_ASSERT((sqes_alloc_ != nullptr));
  ZX_ASSERT((ring_fd >= 0));

  ZX_ERR_TRY((zx_sys_munmap(sqes_alloc_, sqes_sz, err_stack)), err_stack);
  ZX_ERR_TRY((zx_sys_munmap(ring_alloc_, ring_sz, err_stack)), err_stack);
  ZX_ERR_TRY((zx_sys_close((zx_i32)ring_fd_, err_stack)), err_stack);

  __builtin_memset(this, 0, sizeof(FioUring));

  return ZXStatus::Ok;
}
