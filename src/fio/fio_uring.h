#ifndef FIO_URING_H
#define FIO_URING_H

#include <zx/zx_alloc.h>
#include <zx/zx_branch.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_queue.h>
#include <zx/zx_slab.h>
#include <zx/zx_sys.h>
#include <zx/zx_sys_io_uring.h>

static_assert(sizeof(zx_u64) == sizeof(void *));

typedef struct io_uring_sqe fio_sqe;
typedef struct io_uring_cqe fio_cqe;

typedef struct FioUringSys {
  zx_u32 const *sq_kflags;
  zx_u32 const *sq_khead;
  zx_u32 *sq_ktail;
  zx_u32 *cq_khead;
  zx_u32 const *cq_ktail;
  void *ring_alloc;
  void *sqes_alloc;
  fio_cqe const *cqes;
  fio_sqe *sqes;
  zx_u32 sq_head;
  zx_u32 sq_tail;
  zx_u32 cq_head;
  zx_u32 cq_tail;
  zx_u32 ring_alloc_size;
  zx_u32 sqes_alloc_size;
  zx_i32 ring_fd;
  zx_u32 sq_mask;
  zx_u32 sq_n_entries;
  zx_u32 cq_mask;
  bool polled_io;
} FioUringSys;

static inline ZXStatus FioUringSys_init(bool polled_io, zx_i32 wq_fd,
                                        FioUringSys *fio_ring,
                                        ZXErrStack *err_stack) {
  ZX_ASSERT((fio_ring != nullptr));
  ZX_ASSERT((err_stack != nullptr));

  void *ring = nullptr;
  void *sqes = nullptr;
  zx_i32 ring_fd = -1;
  bool registered_napi = false;
  ZXStatus error_status = ZXStatus_Ok;

  zx_u32 flags = IORING_SETUP_SQPOLL | IORING_SETUP_SINGLE_ISSUER |
                 IORING_SETUP_NO_SQARRAY;

  if (polled_io) {
    flags |= IORING_SETUP_IOPOLL;
  }

  if (wq_fd > -1) {
    flags |= IORING_SETUP_ATTACH_WQ;
  }

  struct io_uring_params ring_params;
  __builtin_memset(&ring_params, 0, sizeof(struct io_uring_params));

  ring_params.wq_fd = (zx_u32)wq_fd;
  ring_params.sq_thread_idle = 1000;
  ring_params.flags = flags;

  zx_u32 const n_entries = 64;

  ZX_ERR_TRY(
      (zx_sys_io_uring_setup(n_entries, &ring_params, &ring_fd, err_stack)),
      err_stack);
  ZX_ASSERT((ring_fd >= 0));

  if ((ring_params.features & IORING_FEAT_SINGLE_MMAP) == 0) {
    error_status = ZXStatus_AssertFail;
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  zx_u32 const sring_sz =
      zx_chk_add_u32(ring_params.sq_off.array,
                     zx_chk_mul_u32(ring_params.sq_entries, sizeof(zx_u32)));

  // sanity check. it should always be 16
  static_assert(sizeof(fio_cqe) == 16);

  zx_u32 const cring_sz =
      zx_chk_add_u32(ring_params.cq_off.cqes,
                     zx_chk_mul_u32(ring_params.cq_entries, sizeof(fio_cqe)));

  zx_u32 const ring_sz = zx_int_max_u32(sring_sz, cring_sz);

  error_status = zx_sys_mmap(0, ring_sz, ZX_SYS_PROT_READ | ZX_SYS_PROT_WRITE,
                             ZX_SYS_MAP_SHARED | ZX_SYS_MAP_POPULATE, ring_fd,
                             IORING_OFF_SQ_RING, (zx_u8 **)&ring, err_stack);
  if (error_status != ZXStatus_Ok) {
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  // sanity check. it should always be 64
  static_assert(sizeof(fio_sqe) == 64);

  zx_u32 const sqes_sz =
      zx_chk_mul_u32(ring_params.sq_entries, sizeof(fio_sqe));
  error_status = zx_sys_mmap(0, sqes_sz, ZX_SYS_PROT_READ | ZX_SYS_PROT_WRITE,
                             ZX_SYS_MAP_SHARED | ZX_SYS_MAP_POPULATE, ring_fd,
                             IORING_OFF_SQES, (zx_u8 **)&sqes, err_stack);
  if (error_status != ZXStatus_Ok) {
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  // check features
  if ((ring_params.features & IORING_FEAT_NODROP) == 0) {
    error_status = ZXStatus_AssertFail;
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }
  if ((ring_params.features & IORING_FEAT_SQPOLL_NONFIXED) == 0) {
    error_status = ZXStatus_AssertFail;
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }
  if ((ring_params.features & IORING_FEAT_RSRC_TAGS) == 0) {
    error_status = ZXStatus_AssertFail;
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  if (!polled_io) {
    struct io_uring_napi napi_params;
    __builtin_memset(&napi_params, 0, sizeof(napi_params));
    napi_params.busy_poll_to = 100000;
    napi_params.prefer_busy_poll = 1;

    error_status = zx_sys_io_uring_register(ring_fd, IORING_REGISTER_NAPI,
                                            &napi_params, 1, err_stack);
    if (error_status != ZXStatus_Ok) {
      ZX_ERR_APPEND_LOC(err_stack);
      goto err;
    }
    registered_napi = true;
  }

  zx_u32 *sq_khead = ring + ring_params.sq_off.head;
  zx_u32 *sq_ktail = ring + ring_params.sq_off.tail;
  zx_u32 *cq_khead = ring + ring_params.cq_off.head;
  zx_u32 *cq_ktail = ring + ring_params.cq_off.tail;

  // setup output
  *fio_ring = (FioUringSys){
      .sq_kflags = ring + ring_params.sq_off.flags,
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
      .cqes = ring + ring_params.cq_off.cqes,
      .sqes = ring + ring_params.sq_off.ring_entries,
      .ring_alloc_size = ring_sz,
      .sqes_alloc_size = sqes_sz,
      .ring_fd = ring_fd,
      .sq_mask = *(zx_u32 const *)(ring + ring_params.sq_off.ring_mask),
      .sq_n_entries = ring_params.sq_entries,
      .cq_mask = *(zx_u32 const *)(ring + ring_params.cq_off.ring_mask),
      .polled_io = polled_io,
  };

  return ZXStatus_Ok;
err:
  if (registered_napi) {
    ZX_ASSERT((ring_fd != -1));
    struct io_uring_napi napi_params;
    __builtin_memset(&napi_params, 0, sizeof(napi_params));
    ZX_ERR_TRY((zx_sys_io_uring_register(ring_fd, IORING_UNREGISTER_NAPI,
                                         &napi_params, 1, err_stack)),
               err_stack);
  }

  if (sqes != nullptr) {
    ZX_ERR_TRY_FATAL((zx_sys_munmap(sqes, sqes_sz, err_stack)), err_stack);
  }
  if (ring != nullptr) {
    ZX_ERR_TRY_FATAL((zx_sys_munmap(ring, ring_sz, err_stack)), err_stack);
  }
  if (ring_fd != -1) {
    ZX_ERR_TRY_FATAL((zx_sys_close(ring_fd, err_stack)), err_stack);
  }

  ZX_ASSERT((error_status != ZXStatus_Ok));
  ZX_ERR_TRY(error_status, err_stack);

  ZX_ASSERT((false));
  return ZXStatus_Ok;
}

static inline ZXStatus FioUringSys_deinit(FioUringSys *fio_ring,
                                          ZXErrStack *err_stack) {
  ZX_ASSERT((fio_ring != nullptr));

  if (!fio_ring->polled_io) {
    struct io_uring_napi napi_params;
    __builtin_memset(&napi_params, 0, sizeof(napi_params));
    ZX_ERR_TRY(
        (zx_sys_io_uring_register(fio_ring->ring_fd, IORING_UNREGISTER_NAPI,
                                  &napi_params, 1, err_stack)),
        err_stack);
  }

  void *const ring = fio_ring->ring_alloc;
  zx_u32 const ring_sz = fio_ring->ring_alloc_size;
  void *const sqes = fio_ring->sqes_alloc;
  zx_u32 const sqes_sz = fio_ring->sqes_alloc_size;
  zx_i32 const ring_fd = fio_ring->ring_fd;

  ZX_ASSERT((ring != nullptr));
  ZX_ASSERT((sqes != nullptr));
  ZX_ASSERT((ring_fd >= 0));

  ZX_ERR_TRY((zx_sys_munmap(sqes, sqes_sz, err_stack)), err_stack);
  fio_ring->sqes_alloc = nullptr;

  ZX_ERR_TRY((zx_sys_munmap(ring, ring_sz, err_stack)), err_stack);
  fio_ring->ring_alloc = nullptr;

  ZX_ERR_TRY((zx_sys_close(ring_fd, err_stack)), err_stack);
  fio_ring->ring_fd = -1;

  return ZXStatus_Ok;
}

// Synchronizes the ring with kernel side. Makes io_uring_enter syscall if
// needed.
//
// References to cqe or sqe shoudn't be accessed across a call to this function.
static inline ZXStatus FioUringSys_sync(FioUringSys *fio_ring,
                                        ZXErrStack *err_stack) {
  static_assert(sizeof(unsigned) == sizeof(zx_u32));
  static_assert(alignof(unsigned) == alignof(zx_u32));

  __atomic_store_n((unsigned *)fio_ring->sq_ktail, (unsigned)fio_ring->sq_tail,
                   __ATOMIC_RELEASE);
  __atomic_load((unsigned const *)fio_ring->sq_khead,
                (unsigned *)&fio_ring->sq_head, __ATOMIC_ACQUIRE);
  __atomic_store_n((unsigned *)fio_ring->cq_khead, (unsigned)fio_ring->cq_head,
                   __ATOMIC_RELEASE);
  __atomic_load((unsigned const *)fio_ring->cq_ktail,
                (unsigned *)&fio_ring->cq_tail, __ATOMIC_ACQUIRE);

  zx_u32 const sq_flags = (zx_u32)__atomic_load_n(
      (unsigned const *)fio_ring->sq_kflags, __ATOMIC_RELAXED);
  if (zx_unlikely((sq_flags & IORING_SQ_NEED_WAKEUP) > 0)) {
    zx_u32 const enter_flags = IORING_ENTER_SQ_WAKEUP;

    ZX_ERR_TRY((zx_sys_io_uring_enter(fio_ring->ring_fd, 0, 0, enter_flags,
                                      nullptr, 0, err_stack)),
               err_stack);
  }

  return ZXStatus_Ok;
}

typedef struct FioUringCompletion {
  zx_u64 user_data;
  zx_i32 res;
  zx_u32 flags;
} FioUringCompletion;
static_assert(sizeof(FioUringCompletion) == 16);

// Populates user_data and returns true if there was a completion to get.
// Returns false and doesn't touch user_data otherwise.
static inline bool
FioUringSys_get_next_completion(FioUringSys *fio_ring,
                                FioUringCompletion *completion) {
  if (fio_ring->cq_tail != fio_ring->cq_head) {
    fio_cqe const cqe = fio_ring->cqes[fio_ring->cq_head & fio_ring->cq_mask];
    *completion = (FioUringCompletion){
        .user_data = cqe.user_data,
        .res = cqe.res,
        .flags = cqe.flags,
    };
    fio_ring->cq_head += 1;
    return true;
  } else {
    return false;
  }
}

static inline fio_sqe *FioUringSys_get_next_sqe_(FioUringSys *fio_ring) {
  if (fio_ring->sq_tail - fio_ring->sq_head < fio_ring->sq_n_entries) {
    fio_sqe *ret = &fio_ring->sqes[fio_ring->sq_tail & fio_ring->sq_mask];
    fio_ring->sq_tail += 1;
    return ret;
  } else {
    return nullptr;
  }
}

static inline bool FioUringSys_submit_rw_(FioUringSys *fio_ring, zx_i32 op,
                                          zx_i32 fd, zx_u64 buf_addr,
                                          zx_u64 offset, zx_u32 size) {
  fio_sqe *sqe = FioUringSys_get_next_sqe_(fio_ring);
  if (zx_unlikely(sqe == nullptr)) {
    return false;
  }
  sqe->opcode = (zx_u8)op;
  sqe->fd = fd;
  sqe->addr = buf_addr;
  sqe->off = offset;
  sqe->len = size;

  return true;
}

static inline bool FioUringSys_submit_read(FioUringSys *fio_ring, zx_i32 fd,
                                           zx_u8 *buf, zx_u64 offset,
                                           zx_u32 size) {
  return FioUringSys_submit_rw_(fio_ring, IORING_OP_READ, fd, (zx_u64)buf,
                                offset, size);
}

static inline bool FioUringSys_submit_write(FioUringSys *fio_ring, zx_i32 fd,
                                            zx_u8 const *buf, zx_u64 offset,
                                            zx_u32 size) {
  return FioUringSys_submit_rw_(fio_ring, IORING_OP_WRITE, fd, (zx_u64)buf,
                                offset, size);
}

#endif
