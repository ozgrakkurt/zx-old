#ifndef FIO_FIB_H
#define FIO_FIB_H

#include <fiber/fiber.h>
#include <fio/fio_fut.h>
#include <zx/zx_assert.h>

typedef struct FioFiberCtx {
  FiberCtx const *exe_fib;
  FiberCtx *self_fib;
  FioCtx const *fio_ctx;
  zx_u32 self_id;
} FioFiberCtx;

typedef struct FioFiberHandle {
  FioFiberEntry *entry;
} FioFibeHandle;

static inline void FioFiberHandle_join(FioCtx const *fio_ctx,
                                       FioFiberHandle *handle) {}

static inline void FioFiberHandle_detach(FioCtx const *fio_ctx,
                                         FioFiberHandle *handle) {
  (void)fio_ctx;
  handle->entry->parent_id = ZX_U32_MAX;
}

// typedef void fiber_startfn(FiberCtx const *parent_ctx, FiberCtx *fiber_ctx,
//                            void *usr_ctx);
//
// void fiber_start(FiberCtx *prev, FiberCtx *fiber_ctx, void *usr_ctx,
//                  __attribute__((__noreturn__)) fiber_startfn startfn,
//                  zx_u8 *fiber_stack);
//
// void fiber_switch(FiberCtx *prev, FiberCtx const *next);
//

static inline ZXStatus fio_spawn(FioCtx const *fio_ctx, void *usr_ctx,
                                 fio_fibfn fiber_func, FioFiberEntry *entry,
                                 ZXErrStack *err_stack) {

  return ZXStatus_Ok;
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
