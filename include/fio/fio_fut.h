#ifndef FIO_FUT_H
#define FIO_FUT_H

#include <fio/fio_uring.h>
#include <fio/fio_wake.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>
#include <zx/zx_slab.h>
#include <zx/zx_time.h>

typedef struct FioFutCtx FioFutCtx;

typedef struct FioFuture {
  void *state;
  // returns true if the future is done (ready to be deleted).
  bool (*poll)(FioFutCtx const *fio_ctx, void *ctx);
} FioFuture;

constexpr zx_u8 FIO_IO_PER_FUTURE = 64;

typedef struct FioFutureEntry {
  FioFuture fut;
  // Fiber to wake after this future is done.
  // ZX_U32_MAX means don't wake a fiber.
  zx_u32 fib_to_wake;
  // Future to wake after this future is done.
  // ZX_U32_MAX means don't wake a future.
  zx_u32 fut_to_wake;
} FioFutureEntry;

// for passing context into futures
typedef struct FioFutCtx {
  // io_uring interface for regular IO
  FioUring *io_ring;
  // io_uring interface for Direct IO
  FioUring *polled_io_ring;
  // book-keeping for fibers
  FioWake *fiber_wake;
  ZXSlab *fibers;
  zx_u8 *fiber_stacks;
  // book-keeping for Future objects
  FioWake *future_wake;
  ZXSlab *futures;
  zx_i32 *io_results;
  zx_u64 yield_after_n_ticks;
  // changes per future_poll
  ZXTimer timer;
  zx_u32 fut_id;
  FioFutureEntry *fut_entry;
} FioFutCtx;

#endif
