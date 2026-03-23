#ifndef FIO_FIBER_H
#define FIO_FIBER_H

#include <fio/fio_uring.h>
#include <fio/fio_wake.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_slab.h>

// for passing context into futures

static inline ZXStatus fio_spawn(FioCtx const *ctx) {}

static inline ZXStatus fio_spawn_future(FioCtx const *ctx, FioFuture fut,
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
