#ifndef FIO_URING_H
#define FIO_URING_H

#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_queue.h>
#include <zx/zx_slab.h>
#include <zx/zx_sys.h>
#include <zx/zx_sys_io_uring.h>

void fio_uring_setup_ring_pointers_(struct io_uring_params const *p,
                                    struct io_uring_sq *sq,
                                    struct io_uring_cq *cq) {
  sq->khead = sq->ring_ptr + p->sq_off.head;
  sq->ktail = sq->ring_ptr + p->sq_off.tail;
  sq->kring_mask = sq->ring_ptr + p->sq_off.ring_mask;
  sq->kring_entries = sq->ring_ptr + p->sq_off.ring_entries;
  sq->kflags = sq->ring_ptr + p->sq_off.flags;
  sq->kdropped = sq->ring_ptr + p->sq_off.dropped;
  if (!(p->flags & IORING_SETUP_NO_SQARRAY))
    sq->array = sq->ring_ptr + p->sq_off.array;

  cq->khead = cq->ring_ptr + p->cq_off.head;
  cq->ktail = cq->ring_ptr + p->cq_off.tail;
  cq->kring_mask = cq->ring_ptr + p->cq_off.ring_mask;
  cq->kring_entries = cq->ring_ptr + p->cq_off.ring_entries;
  cq->koverflow = cq->ring_ptr + p->cq_off.overflow;
  cq->cqes = cq->ring_ptr + p->cq_off.cqes;
  if (p->cq_off.flags)
    cq->kflags = cq->ring_ptr + p->cq_off.flags;

  sq->ring_mask = *sq->kring_mask;
  sq->ring_entries = *sq->kring_entries;
  cq->ring_mask = *cq->kring_mask;
  cq->ring_entries = *cq->kring_entries;
}

typedef struct FioUring {
  struct io_uring_sq sq;
  struct io_uring_cq cq;
  void *ring;
  void *sqes;
  zx_u32 ring_size;
  zx_u32 sqes_size;
  zx_i32 ring_fd;
  ZXQueue io_queue;
  ZXSlab slab;
  bool is_polled;
} FioUring;

typedef struct FioUringParams {
  bool polled_io;
  zx_i32 wq_fd;
} FioUringParams;

static inline ZXStatus FioUring_init(ZXAlloc *alloc,
                                     FioUringParams const *params,
                                     FioUring *fio_ring,
                                     ZXErrStack *err_stack) {
  void *ring = nullptr;
  void *sqes = nullptr;
  zx_i32 ring_fd = -1;
  ZXStatus error_status = ZXStatus_Ok;

  zx_u32 flags = IORING_SETUP_SQPOLL | IORING_SETUP_SINGLE_ISSUER |
                 IORING_SETUP_NO_SQARRAY;

  if (params->polled_io) {
    flags |= IORING_SETUP_IOPOLL;
  }

  zx_u32 const wq_fd = params->wq_fd;

  if (wq_fd != 0) {
    flags |= IORING_SETUP_ATTACH_WQ;
  }

  struct io_uring_params ring_params;
  __builtin_memset(&ring_params, 0, sizeof(struct io_uring_params));

  ring_params.wq_fd = wq_fd;
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
  static_assert(sizeof(struct io_uring_cqe) == 16);

  zx_u32 const cring_sz = zx_chk_add_u32(
      ring_params.cq_off.cqes,
      zx_chk_mul_u32(ring_params.cq_entries, sizeof(struct io_uring_cqe)));

  zx_u32 const ring_sz = zx_int_max_u32(sring_sz, cring_sz);

  error_status = zx_sys_mmap(0, ring_sz, ZX_SYS_PROT_READ | ZX_SYS_PROT_WRITE,
                             ZX_SYS_MAP_SHARED | ZX_SYS_MAP_POPULATE, ring_fd,
                             IORING_OFF_SQ_RING, &ring_ptr, err_stack);
  if (error_status != ZXStatus_Ok) {
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  // sanity check. it should always be 64
  static_assert(sizeof(struct io_uring_sqe), 64);

  zx_u32 const sqes_sz =
      zx_chk_mul_u32(ring_params.sq_entries, sizeof(struct io_uring_sqe));
  error_status = zx_sys_mmap(0, sqes_sz, ZX_SYS_PROT_READ | ZX_SYS_PROT_WRITE,
                             ZX_SYS_MAP_SHARED | ZX_SYS_MAP_POPULATE, ring_fd,
                             IORING_OFF_SQ_RING, &sqes, err_stack);
  if (error_status != ZXStatus_Ok) {
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  // check features
  // ring_param.features

  // setup output
  __builtin_memset(&fio_ring, 0, sizeof(FioUring));
  fio_uring_setup_ring_pointers_(&ring_params, &fio_ring->sq, &fio_ring->cq);
  fio_ring->ring = ring;
  fio_ring->ring_size = ring_sz;
  fio_ring->sqes = sqes;
  fio_ring->sqes_size = sqes_sz;
  fio_ring->ring_fd = ring_fd;
  fio_ring->is_polled = params->polled_io;

  // struct io_uring_sq sq;
  // struct io_uring_cq cq;
  // void* ring;
  // void* sqes;
  // zx_i32 ring_fd;
  // ZXQueue io_queue;
  // ZXSlab slab;
  // bool is_polled;

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

static inline ZXStatus FioUring_deinit(FioUring *fio_ring,
                                       ZXErrStack *err_stack) {
  ZX_ASSERT((fio_ring != nullptr));

  void *const ring = fio_ring->ring;
  zx_u32 const ring_sz = fio_ring->ring_size;
  void *const sqes = fio_ring->sqes;
  zx_u32 const sqes_sz = fio_ring->sqes_size;
  zx_i32 const ring_fd = fio_ring->ring_fd;

  ZX_ASSERT((ring != nullptr));
  ZX_ASSERT((sqes != nullptr));
  ZX_ASSERT((ring_fd >= 0));

  ZX_ERR_TRY((zx_sys_munmap(ring, ring_sz, err_stack)), err_stack);
  fio_ring->ring = nullptr;

  ZX_ERR_TRY((zx_sys_munmap(sqes, sqes_sz, err_stack)), err_stack);
  fio_ring->sqes = nullptr;

  ZX_ERR_TRY((zx_sys_close(ring_fd, err_stack)), err_stack);
  fio_ring->ring_fd = -1;

  return ZXStatus_Ok;
}

#endif
