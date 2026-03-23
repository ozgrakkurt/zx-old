#ifndef FIO_IMPL_H
#define FIO_IMPL_H

#include <fiber/fiber.h>
#include <fio/fio.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_slab.h>

// Entry is used by the executor to keep info about a Fiber.
// Entry is also used by both the executor and the fiber itself for
// communicating with each other.
typedef struct FioFiberEntry {
  FiberCtx self_fib_ctx;
  zx_u64 io_is_finished;
  zx_u64 io_is_pending;
} FioFiberEntry;

// Copied into fiber stack at statup of a fiber and re-used by the running
// fiber.
typedef struct Fio {
  // ---- Global ----
  FioUring *io_ring;
  FioUring *polled_io_ring;
  zx_i32 *io_results;
  FioWake *wake;
  FiberCtx const *exe_fib_ctx;
  zx_u64 yield_after_n_ticks;
  // ---- Fiber specific ----
  FioFiberEntry *entry;
  zx_u32 fiber_id;
} Fio;

// typedef void (*fio_fiberfn)(Fio const *fio, void *usr_ctx);
// ZXStatus fio_spawn(Fio const *fio, void *usr_ctx, fio_fiberfn fiber_func,
// ZXErrStack *err_stack); void fio_yield_if_needed(Fio const *fio);

typedef struct FioTrampoline {
  Fio fio;
  void *usr_ctx;
  fio_fiberfn fiber_func;
} FioTampoline;

// Needs to match fiber_startfn function interface.
// Used for starting a fiber defined by user.
__attribute__((__noreturn__)) static inline void
fio_trampoline_(FiberCtx const *exe_ctx, FiberCtx *self_ctx,
                void *fio_trampoline) {
  FioTrampoline const t = *(FioTrampoline const *)fio_trampoline;

  t.fiber_func(&t.fio, t.usr_ctx);

  // Mark fiber as done and yield back to the executor.
  t.fio_entry->is_done = true;
  fiber_switch(self_ctx, exe_ctx);

  ZX_ASSERT((false));
  __builtin_unreachable();
}

#endif
