#ifndef FIO_URING_H
#define FIO_URING_H

#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>
#include <zx/zx_sys.h>
#include <zx/zx_sys_io_uring.h>

static_assert(sizeof(zx_u64) == sizeof(void *));

typedef io_uring_sqe FioSQE;
typedef io_uring_cqe FioCQE;

static_assert(sizeof(unsigned) == sizeof(zx_u32));
static_assert(alignof(unsigned) == alignof(zx_u32));

struct FioUringParams {
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
};

struct FioUring {
  zx_u32 const *sq_kflags;
  zx_u32 const *sq_khead;
  zx_u32 *sq_ktail;
  zx_u32 *cq_khead;
  zx_u32 const *cq_ktail;
  zx_u8 *ring_alloc;
  zx_u8 *sqes_alloc;
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
  zx_u32 wq_fd_;
  bool registered_buffer;
  bool polled_io;

  static ZXStatus init(FioUringParams const *params, FioUring *fio_ring,
                       ZXErrStack *err_stack);
  ZXStatus deinit(ZXErrStack *err_stack);

  inline void sync_sq() {
    __atomic_store_n((unsigned *)this->sq_ktail, (unsigned)this->sq_tail,
                     __ATOMIC_RELEASE);
    __atomic_load((unsigned const *)this->sq_khead, (unsigned *)&this->sq_head,
                  __ATOMIC_ACQUIRE);
  }

  inline void sync_cq() {
    __atomic_store_n((unsigned *)this->cq_khead, (unsigned)this->cq_head,
                     __ATOMIC_RELEASE);
    __atomic_load((unsigned const *)this->cq_ktail, (unsigned *)&this->cq_tail,
                  __ATOMIC_ACQUIRE);
  }

  // Synchronizes the ring with kernel side. Makes io_uring_enter syscall if
  // needed.
  //
  // References to cqe or sqe shoudn't be accessed across a call to this
  // function.
  inline ZXStatus sync(ZXErrStack *err_stack) {
    static_assert(sizeof(unsigned) == sizeof(zx_u32));
    static_assert(alignof(unsigned) == alignof(zx_u32));

    this->sync_sq();
    this->sync_cq();

    zx_u32 const sq_flags = (zx_u32)__atomic_load_n(
        (unsigned const *)this->sq_kflags, __ATOMIC_RELAXED);
    if ((sq_flags & IORING_SQ_NEED_WAKEUP) > 0) [[unlikely]] {
      zx_u32 const enter_flags = IORING_ENTER_SQ_WAKEUP;

      ZX_ASSERT((this->ring_fd > 0));
      ZX_ERR_TRY((zx_sys_io_uring_enter((zx_i32)this->ring_fd, 0, 0,
                                        enter_flags, nullptr, 0, err_stack)),
                 err_stack);
    }

    return ZXStatus::Ok;
  }

  inline zx_u32 wq_fd() const {
    zx_u32 const wq_fd_v = this->ring_fd;
    ZX_ASSERT(wq_fd_v > 0);
    return wq_fd_v;
  }

  // Returns pointer to next cqe and advances the completion queue.
  // Returns nullptr if the completion queue is empty.
  // The returned pointer should be processed and discarded immediately after
  // calling this function. It is illegal to call any other method in the uring
  // object until the returned pointer is discarded.
  inline FioCQE const *pop_cqe_in_place() {
    if (this->cq_tail == this->cq_head) {
      // Check if kernel pushed more entries so we pop more.
      this->sync_cq();
    }

    if (this->cq_tail != this->cq_head) {
      FioCQE const *ret = &this->cqes[this->cq_head & this->cq_mask];
      this->cq_head += 1;
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
  inline FioSQE *push_sqe_in_place() {
    if (this->sq_tail - this->sq_head >= this->sq_n_entries) {
      // Check if kernel processed more entries so we can push more.
      this->sync_sq();
    }

    if (this->sq_tail - this->sq_head < this->sq_n_entries) {
      FioSQE *ret = &this->sqes[this->sq_tail & this->sq_mask];
      this->sq_tail += 1;
      return ret;
    } else {
      return nullptr;
    }
  }
};

static inline void fio_uring_prep_rw_(FioSQE *sqe, zx_i32 op, zx_i32 fd,
                                      zx_u64 buf_addr, zx_u64 offset,
                                      zx_u32 size) {
  sqe->opcode = (zx_u8)op;
  sqe->fd = fd;
  sqe->addr = buf_addr;
  sqe->off = offset;
  sqe->len = size;
}

static inline void fio_uring_prep_read(FioSQE *sqe, zx_i32 fd, zx_u8 *buf,
                                       zx_u64 offset, zx_u32 size) {
  fio_uring_prep_rw_(sqe, IORING_OP_READ, fd, (zx_u64)buf, offset, size);
}

static inline void fio_uring_prep_write(FioSQE *sqe, zx_i32 fd,
                                        zx_u8 const *buf, zx_u64 offset,
                                        zx_u32 size) {
  fio_uring_prep_rw_(sqe, IORING_OP_WRITE, fd, (zx_u64)buf, offset, size);
}

static inline void fio_uring_prep_read_fixed(FioSQE *sqe, zx_i32 fd, zx_u8 *buf,
                                             zx_u64 offset, zx_u32 size) {
  fio_uring_prep_rw_(sqe, IORING_OP_READ_FIXED, fd, (zx_u64)buf, offset, size);
  sqe->buf_index = 0;
}

static inline void fio_uring_prep_write_fixed(FioSQE *sqe, zx_i32 fd,
                                              zx_u8 const *buf, zx_u64 offset,
                                              zx_u32 size) {
  fio_uring_prep_rw_(sqe, IORING_OP_WRITE_FIXED, fd, (zx_u64)buf, offset, size);
  sqe->buf_index = 0;
}

#endif
