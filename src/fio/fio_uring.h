#ifndef FIO_URING_H
#define FIO_URING_H

#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_sys.h>
#include <zx/zx_sys_io_uring.h>

static_assert(sizeof(zx_u64) == sizeof(void *));

typedef struct io_uring_sqe fio_sqe;
typedef struct io_uring_cqe fio_cqe;

typedef struct FioUring {
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
} FioUring;

ZXStatus FioUring_init(bool polled_io, zx_i32 wq_fd, FioUring *fio_ring,
                       ZXErrStack *err_stack);

ZXStatus FioUring_deinit(FioUring *fio_ring, ZXErrStack *err_stack);

// Synchronizes the ring with kernel side. Makes io_uring_enter syscall if
// needed.
//
// References to cqe or sqe shoudn't be accessed across a call to this function.
ZXStatus FioUring_sync(FioUring *fio_ring, ZXErrStack *err_stack);

static inline zx_i32 FioUring_wq_fd(FioUring const *fio_uring) {
  return fio_uring->ring_fd;
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
FioUring_get_next_completion(FioUring *fio_ring,
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

static inline fio_sqe *FioUring_get_next_sqe_(FioUring *fio_ring) {
  if (fio_ring->sq_tail - fio_ring->sq_head < fio_ring->sq_n_entries) {
    fio_sqe *ret = &fio_ring->sqes[fio_ring->sq_tail & fio_ring->sq_mask];
    fio_ring->sq_tail += 1;
    return ret;
  } else {
    return nullptr;
  }
}

static inline bool FioUring_submit_rw_(FioUring *fio_ring, zx_i32 op, zx_i32 fd,
                                       zx_u64 buf_addr, zx_u64 offset,
                                       zx_u32 size) {
  fio_sqe *sqe = FioUring_get_next_sqe_(fio_ring);
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

static inline bool FioUring_submit_read(FioUring *fio_ring, zx_i32 fd,
                                        zx_u8 *buf, zx_u64 offset,
                                        zx_u32 size) {
  return FioUring_submit_rw_(fio_ring, IORING_OP_READ, fd, (zx_u64)buf, offset,
                             size);
}

static inline bool FioUring_submit_write(FioUring *fio_ring, zx_i32 fd,
                                         zx_u8 const *buf, zx_u64 offset,
                                         zx_u32 size) {
  return FioUring_submit_rw_(fio_ring, IORING_OP_WRITE, fd, (zx_u64)buf, offset,
                             size);
}

#endif
