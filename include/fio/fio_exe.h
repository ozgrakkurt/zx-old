#ifndef FIO_EXE_H
#define FIO_EXE_H

#include <fiber/fiber.h>
#include <fio/fio_fut.h>
#include <fio/fio_uring.h>
#include <fio/fio_wake.h>
#include <zx/zx_alloc.h>
#include <zx/zx_bitmap.h>
#include <zx/zx_err.h>
#include <zx/zx_slab.h>
#include <zx/zx_time.h>

struct FioExeParams {
  // Handle for sharing kernel SQ_POLL between different instances
  // 0 means no handle.
  zx_u32 wq_fd;
  // Maximum number of fibers that can be running at a time.
  // Attempting to spawn more fibers will error.
  zx_u32 max_n_fibers;
  // Maximum number of futures that can be running at a time.
  // Attempting to spawn more futures will error.
  zx_u32 max_n_futures;
  // IO buffer for registering with io_uring. All later buffers passed to do io
  // operations have to be inside the range of this buffer. passing nullptr for
  // io_buf and zero for io_buf_size means "don't register anything". In that
  // case, it is not possible to do io operations that require passing a buffer
  // including file read/write and socket read/write.
  zx_u8 *io_buf;
  zx_u64 io_buf_size;
  // yield_if_needed will return true after a future has been running this
  // number of ticks.
  zx_u64 yield_after_n_ticks;
};

struct FioExe {
  // io_uring interface for regular IO
  FioUring io_ring;
  // io_uring interface for Direct IO
  FioUring polled_io_ring;
  // book-keeping for fibers
  FioWake fiber_wake;
  // Slab(Fiber)
  ZXSlab fibers;
  zx_u8 *fiber_stacks;
  // book-keeping for Future objects
  FioWake future_wake;
  // Slab(FutureEntry)
  ZXSlab futures;
  zx_i32 *io_results;
  zx_u64 yield_after_n_ticks;
};

ZXStatus FioExe_init(ZXAlloc *alloc, FioExeParams const *params, FioExe *exe,
                     ZXErrStack *err_stack);

ZXStatus FioExe_deinit(FioExe *exe, ZXErrStack *err_stack);

static inline zx_u32 FioExe_wq_fd(FioExe const *exe) {
  return FioUring_wq_fd(&exe->io_ring);
}

#endif
