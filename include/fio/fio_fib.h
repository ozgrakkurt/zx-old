#ifndef FIO_FIB_H
#define FIO_FIB_H

#include <fiber/fiber.h>
#include <fio/fio_fut.h>

typedef struct FioFibCtx {
  FiberCtx parent;
} FioFibCtx;

// typedef void fiber_startfn(FiberCtx const *parent_ctx, FiberCtx *fiber_ctx,
//                            void *usr_ctx);
//
// void fiber_start(FiberCtx *prev, FiberCtx *fiber_ctx, void *usr_ctx,
//                  __attribute__((__noreturn__)) fiber_startfn startfn,
//                  zx_u8 *fiber_stack);
//
// void fiber_switch(FiberCtx *prev, FiberCtx const *next);
//

typedef struct FioFibStartCtx {
  FioFibCtx const *fio_ctx;
  void *usr_ctx;
  void (*fibfn)(FioFibCtx const *fio_ctx, void *usr_ctx);
  bool is_done;
} FioFibStartCtx;

__attribute__((__noreturn__)) static inline void
fio_fib_start(FiberCtx const *parent_ctx, FiberCtx *fiber_ctx,
              void *start_ctx) {
  FioFibStartCtx const *c = (FioFibStartCtx const *)start_ctx;
  c->fibfn(c->fio_ctx, c->usr_ctx);
  c->is_done = true;
  fiber_switch(fiber_ctx, parent_ctx);
}

typedef struct FioAwait {
  FioFuture inner;
  zx_u32 fib_id;
} FioAwait;

static inline void FioAwait_poll() {}

static inline fio_yield(FioFiberCtx const *fio_ctx) {
  Fiber_switch(fio_ctx->parent, fio_ctx->current);
}

static inline ZXStatus fio_await(FioFiberCtx const *fio_ctx, FioFuture fut) {
  FioAwait aw = {
      .inner = fut,
      .fib_id = FioCtx_fiber_id(fio_ctx),
  };
  FioFuture fut = {
      .state = &aw,
      .poll = FioAwait_poll,
  };
  FioFutCtx_spawn(fio_ctx, aw);
  fio_yield(fio_ctx);
}

static inline void fio_yield_if_needed(FioCtx const *fio_ctx) {
  //
}

#endif
