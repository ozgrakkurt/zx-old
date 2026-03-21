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
  // bitmaps for state.
  // a finished io will have it's result at FioFutCtx.io_results[fut_id *
  // IO_PER_FUTURE + io_id]
  zx_u64 is_finished;
  zx_u64 is_pending;
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

static inline ZXStatus FioFutCtx_spawn(FioFutCtx *ctx, FioFuture fut,
                                       zx_u32 *fut_id, ZXErrStack *err_stack) {
  FioFutureEntry const entry = {
      .fut = fut,
      .is_finished = 0,
      .is_pending = 0,
  };

  ZXSlab *futs = ctx->futures;

  ZX_ERR_ASSERT((ZXSlab_capacity_left(futs) > 0), err_stack);
  zx_u32 f_id = ZXSlab_insert(futs, &entry, sizeof(FioFutureEntry));
  FioWake_set(ctx->future_wake, f_id);
  *fut_id = f_id;

  return ZXStatus_Ok;
}

static inline bool FioFutCtx_yield_if_needed(FioFutCtx *ctx) {
  zx_u64 const elapsed = ZXTimer_elapsed(&ctx->timer);
  if (elapsed > ctx->yield_after_n_ticks) {
    FioWake_set(ctx->future_wake, ctx->fut_id);
    return true;
  } else {
    return false;
  }
}

static inline void FioFutCtx_wake_self(FioFutCtx *ctx) {
  FioWake_set(ctx->future_wake, ctx->fut_id);
}

// It is illegal to call this function when a future already has max number of
// pending/finished io.
// Returns a pointer to sqe entry and marks it as initialized if an empty entry
// can be found at this time. Returned entry should be immediately initialized
// after calling this function. Returns nullptr if an entry can't be found.
// Caller should retry later.
static inline FioSQE *FioFutCtx_push_sqe(FioFutCtx *ctx, bool polled_io) {
  FioFutureEntry *fut_entry = ctx->fut_entry;

  zx_u64 const is_filled = fut_entry->is_pending | fut_entry->is_finished;
  ZX_ASSERT((is_filled != ZX_U64_MAX));

  // bit index of the first zero
  zx_u8 const io_id = zx_chk_sub_u8(zx_ctz_u64(~is_filled), 1);

  FioUring *ring = polled_io ? ctx->polled_io_ring : ctx->io_ring;
  FioSQE *sqe = FioUring_push_sqe(ring);
  if (sqe == nullptr) {
    return nullptr;
  }

  sqe->user_data = ((zx_u64){io_id} << 32) | (zx_u64){ctx->fut_id};

  // mark it pending
  fut_entry->is_pending |= ((zx_u64){1} << io_id);

  return sqe;
}

// returns true if a result was found and stored into `result` out param.
static inline bool FioFutCtx_remove_io_result(FioFutCtx *ctx, zx_u32 io_id,
                                              zx_i32 *result) {
  FioFutureEntry *entry = ctx->fut_entry;

  zx_u64 const mask = (zx_u64){1} << io_id;

  if ((entry->is_finished & mask) != 0) {
    // remove finished marker
    entry->is_finished &= ~mask;
    *result = ctx->io_results[ctx->fut_id * FIO_IO_PER_FUTURE + io_id];
    return true;
  } else {
    return false;
  }
}

#endif
