#ifndef FIO_URING_H
#define FIO_URING_H

#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_sys.h>
#include <zx/zx_sys_io_uring.h>

static_assert(sizeof(zx_u64) == sizeof(void *));

typedef struct io_uring_sqe FioSQE;
typedef struct io_uring_cqe FioCQE;

static_assert(sizeof(unsigned) == sizeof(zx_u32));
static_assert(alignof(unsigned) == alignof(zx_u32));

typedef struct FioUring {
  zx_u32 const *sq_kflags;
  zx_u32 const *sq_khead;
  zx_u32 *sq_ktail;
  zx_u32 *cq_khead;
  zx_u32 const *cq_ktail;
  void *ring_alloc;
  void *sqes_alloc;
  FioCQE const *cqes;
  FioSQE *sqes;
  zx_u32 sq_head;
  zx_u32 sq_tail;
  zx_u32 cq_head;
  zx_u32 cq_tail;
  zx_u32 ring_alloc_size;
  zx_u32 sqes_alloc_size;
  zx_u32 ring_fd;
  zx_u32 sq_mask;
  zx_u32 sq_n_entries;
  zx_u32 cq_mask;
  zx_u32 wq_fd;
  bool registered_buffer;
  bool polled_io;
} FioUring;

typedef struct FioUringParams {
  // IO buffer for registering with io_uring. All later buffers passed to do io
  // operations have to be inside the range of this buffer. passing nullptr for
  // io_buf and zero for io_buf_size means "don't register anything". In that
  // case, it is not possible to do io operations that require passing a buffer
  // including file read/write and socket read/write.
  zx_u8 *io_buf;
  zx_u64 io_buf_size;
  // Value of 0 is ignore, any other value is registered as wq_fd into uring
  // instance used for sharing sqpoll thread for multiple io_uring instances.
  zx_u32 wq_fd;
  // Create a ring for only doing polled direct io.
  bool polled_io;
} FioUringParams;

ZXStatus FioUring_init(FioUringParams const *params, FioUring *fio_ring,
                       ZXErrStack *err_stack);

ZXStatus FioUring_deinit(FioUring *fio_ring, ZXErrStack *err_stack);

// Synchronizes the ring with kernel side. Makes io_uring_enter syscall if
// needed.
//
// References to cqe or sqe shoudn't be accessed across a call to this function.
ZXStatus FioUring_sync(FioUring *fio_ring, ZXErrStack *err_stack);

static inline void FioUring_sync_sq(FioUring *fio_ring) {
  __atomic_store_n((unsigned *)fio_ring->sq_ktail, (unsigned)fio_ring->sq_tail,
                   __ATOMIC_RELEASE);
  __atomic_load((unsigned const *)fio_ring->sq_khead,
                (unsigned *)&fio_ring->sq_head, __ATOMIC_ACQUIRE);
}

static inline void FioUring_sync_cq(FioUring *fio_ring) {
  __atomic_store_n((unsigned *)fio_ring->cq_khead, (unsigned)fio_ring->cq_head,
                   __ATOMIC_RELEASE);
  __atomic_load((unsigned const *)fio_ring->cq_ktail,
                (unsigned *)&fio_ring->cq_tail, __ATOMIC_ACQUIRE);
}

static inline zx_u32 FioUring_wq_fd(FioUring const *fio_ring) {
  zx_u32 const wq_fd = fio_ring->ring_fd;
  ZX_ASSERT(wq_fd > 0);
  return wq_fd;
}

// Returns pointer to next cqe and advances the completion queue.
// Returns nullptr if the completion queue is empty.
// The returned pointer should be processed and discarded immediately after
// calling this function. It is illegal to call any other method in the uring
// object until the returned pointer is discarded.
static inline FioCQE const *FioUring_pop_cqe(FioUring *fio_ring) {
  if (fio_ring->cq_tail == fio_ring->cq_head) {
    // Check if kernel pushed more entries so we pop more.
    FioUring_sync_cq(fio_ring);
  }

  if (fio_ring->cq_tail != fio_ring->cq_head) {
    FioCQE const *ret = &fio_ring->cqes[fio_ring->cq_head & fio_ring->cq_mask];
    fio_ring->cq_head += 1;
    return ret;
  } else {
    return nullptr;
  }
}

// Returns pointer to next sqe and advances the submission queue.
// Returns nullptr if the submission queue is full.
// The returned pointer should be processed and discarded immediately after
// calling this function. It is illegal to call any other method in the uring
// object until the returned pointer is discarded.
static inline FioSQE *FioUring_push_sqe(FioUring *fio_ring) {
  if (fio_ring->sq_tail - fio_ring->sq_head >= fio_ring->sq_n_entries) {
    // Check if kernel processed more entries so we can push more.
    FioUring_sync_sq(fio_ring);
  }

  if (fio_ring->sq_tail - fio_ring->sq_head < fio_ring->sq_n_entries) {
    FioSQE *ret = &fio_ring->sqes[fio_ring->sq_tail & fio_ring->sq_mask];
    fio_ring->sq_tail += 1;
    return ret;
  } else {
    return nullptr;
  }
}

static inline void FioUring_prep_rw_(FioSQE *sqe, zx_i32 op, zx_i32 fd,
                                     zx_u64 buf_addr, zx_u64 offset,
                                     zx_u32 size) {
  sqe->opcode = (zx_u8)op;
  sqe->fd = fd;
  sqe->addr = buf_addr;
  sqe->off = offset;
  sqe->len = size;
}

static inline void FioUring_prep_read_fixed(FioSQE *sqe, zx_i32 fd, zx_u8 *buf,
                                            zx_u64 offset, zx_u32 size) {
  FioUring_prep_rw_(sqe, IORING_OP_READ_FIXED, fd, (zx_u64)buf, offset, size);
  sqe->buf_index = 0;
}

static inline void FioUring_prep_write_fixed(FioSQE *sqe, zx_i32 fd,
                                             zx_u8 const *buf, zx_u64 offset,
                                             zx_u32 size) {
  FioUring_prep_rw_(sqe, IORING_OP_WRITE_FIXED, fd, (zx_u64)buf, offset, size);
  sqe->buf_index = 0;
}

#endif
