#ifndef FIO_EXE_H
#define FIO_EXE_H

#include "./fio_uring.h"
#include "./fio_wake.h"
#include <fiber/fiber.h>
#include <zx/zx_alloc.h>
#include <zx/zx_bitmap.h>
#include <zx/zx_err.h>
#include <zx/zx_slab.h>
#include <zx/zx_time.h>

typedef bool (*fio_fut_pollfn)(FioCtx const *ctx, void *state);
typedef struct FioFuture {
  void *state;
  fio_fut_pollfn pollfn;
} FioFuture;

constexpr zx_u16 FIO_MAX_IO_PER_FUTURE = 32;

typedef struct FioFutureEntry {
  FioFuture fut;
  FioCQE finished_io[FIO_MAX_IO_PER_FUTURE];
  zx_u16 n_finished_io;
  zx_u16 n_pending_io;
} FioFutureEntry;

// for passing context into futures and fibers
typedef struct FioCtx {
  ZXTimer timer;
  zx_u64 preempt_n_ticks;
  zx_u32 fut_id;
  FioFuture *fut;
  FioExe *exe;
} FioCtx;

static inline ZXStatus FioCtx_spawn(FioCtx *ctx, FioFuture fut, zx_u32 *fut_id,
                                    ZXErrStack *err_stack) {
  FioFutureEntry const entry = {
      .fut = fut,
      .finished_io = {0},
      .n_finished_io = 0,
      .n_pending_io = 0,
  };

  ZXSlab *futs = &ctx->exe->futures;

  ZX_ERR_ASSERT((ZXSlab_capacity_left(futs) > 0), err_stack);
  *fut_id = ZXSlab_insert(futs, &entry, sizeof(FioFutureEntry));

  return ZXStatus_Ok;
}

static inline bool FioCtx_yield_if_needed(FioCtx *ctx) {
  zx_u64 const elapsed = ZXTimer_elapsed(&ctx->timer);
  if (elapsed > ctx->preempt_n_ticks) {
    FioWake_set(&ctx->exe->future_wake, ctx->fut_id);
    return true;
  } else {
    return false;
  }
}

static inline void FioCtx_wake_self(FioCtx *ctx) {
  FioWake_set(&ctx->exe->future_wake, ctx->fut_id);
}

typedef struct FioFiber {
  FiberCtx fib_ctx;
} FioFiber;

typedef struct FioExe {
  // io_uring interface for regular IO
  FioUring io_ring;
  // io_uring interface for Direct IO
  FioUring polled_io_ring;
  // book-keeping for fibers
  FioWake fiber_wake;
  ZXSlab fibers;
  zx_u8 *fiber_stacks;
  // book-keeping for Future objects
  FioWake future_wake;
  ZXSlab futures;
} FioExe;

typedef struct FioExeParams {
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
} FioExeParams;

ZXStatus FioExe_init(ZXAlloc *alloc, FioExeParams const *params, FioExe *exe,
                     ZXErrStack *err_stack);

ZXStatus FioExe_deinit(FioExe *exe, ZXErrStack *err_stack);

static inline zx_u32 FioExe_wq_fd(FioExe const *exe) {
  return FioUring_wq_fd(&exe->io_ring);
}

#endif
