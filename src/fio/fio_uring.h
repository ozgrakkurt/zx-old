#ifndef FIO_URING_H
#define FIO_URING_H

#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_queue.h>
#include <zx/zx_slab.h>
#include <zx/zx_sys.h>
#include <zx/zx_sys_io_uring.h>

typedef struct io_uring_sqe fio_sqe;
typedef struct io_uring_cqe fio_cqe;

typedef struct FioUringSys {
  zx_u32 *sq_head;
  zx_u32 *sq_tail;
  zx_u32 *cq_head;
  zx_u32 *cq_tail;
  void *ring_alloc;
  void *sqes_alloc;
  fio_cqe *cqes;
  fio_sqe *sqes;
  zx_u32 ring_alloc_size;
  zx_u32 sqes_alloc_size;
  zx_i32 ring_fd;
  zx_u32 sq_mask;
  zx_u32 sq_n_entries;
  zx_u32 cq_mask;
} FioUringSys;

static inline ZXStatus FioUringSys_init(bool polled_io, zx_i32 wq_fd,
                                        FioUringSys *fio_ring,
                                        ZXErrStack *err_stack) {
  ZX_ASSERT((fio_ring != nullptr));
  ZX_ASSERT((err_stack != nullptr));

  void *ring = nullptr;
  void *sqes = nullptr;
  zx_i32 ring_fd = -1;
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

  zx_u32 const n_entries = 32;

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

  // setup output
  *fio_ring = (FioUringSys){
      .sq_head = ring + ring_params.sq_off.head,
      .sq_tail = ring + ring_params.sq_off.tail,
      .cq_head = ring + ring_params.cq_off.head,
      .cq_tail = ring + ring_params.cq_off.tail,
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
  };

  return ZXStatus_Ok;
err:
  if (ring_fd != -1) {
    ZX_ERR_TRY_FATAL((zx_sys_close(ring_fd, err_stack)), err_stack);
  }
  if (ring != nullptr) {
    ZX_ERR_TRY_FATAL((zx_sys_munmap(ring, ring_sz, err_stack)), err_stack);
  }
  if (sqes != nullptr) {
    ZX_ERR_TRY_FATAL((zx_sys_munmap(sqes, sqes_sz, err_stack)), err_stack);
  }

  ZX_ASSERT((error_status != ZXStatus_Ok));
  ZX_ERR_TRY(error_status, err_stack);

  ZX_ASSERT((false));
  return ZXStatus_Ok;
}

static inline ZXStatus FioUringSys_deinit(FioUringSys *fio_ring,
                                          ZXErrStack *err_stack) {
  ZX_ASSERT((fio_ring != nullptr));

  void *const ring = fio_ring->ring_alloc;
  zx_u32 const ring_sz = fio_ring->ring_alloc_size;
  void *const sqes = fio_ring->sqes_alloc;
  zx_u32 const sqes_sz = fio_ring->sqes_alloc_size;
  zx_i32 const ring_fd = fio_ring->ring_fd;

  ZX_ASSERT((ring != nullptr));
  ZX_ASSERT((sqes != nullptr));
  ZX_ASSERT((ring_fd >= 0));

  ZX_ERR_TRY((zx_sys_munmap(ring, ring_sz, err_stack)), err_stack);
  fio_ring->ring_alloc = nullptr;

  ZX_ERR_TRY((zx_sys_munmap(sqes, sqes_sz, err_stack)), err_stack);
  fio_ring->sqes_alloc = nullptr;

  ZX_ERR_TRY((zx_sys_close(ring_fd, err_stack)), err_stack);
  fio_ring->ring_fd = -1;

  return ZXStatus_Ok;
}

#endif
