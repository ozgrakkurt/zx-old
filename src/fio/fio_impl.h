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
  FiberCtx fib_ctx;
  zx_u64 io_is_finished;
  zx_u64 io_is_pending;
} FioFiberEntry;

// has to be same as bitsize of u64 because we use u64 bitmaps for recording io
// status per fiber.
constexpr zx_u8 FIO_IO_PER_FIBER = sizeof(zx_u64) * 8;

// Copied into fiber stack at statup of a fiber and re-used by the running
// fiber.
typedef struct Fio {
  // ---- Global ----
  FioUring *io_ring;
  FioUring *polled_io_ring;
  zx_i32 *io_results;
  FioWake *wake;
  FiberCtx const *finish_fib_ctx;
  zx_u64 yield_after_n_ticks;
  ZXSlab *fibers;
  // ---- Fiber specific ----
  zx_u32 fiber_id;
} Fio;

static inline void Fio_switch_next(Fio const *fio, FiberCtx *current_fib_ctx,
                                   zx_u32 next_id) {
  FioFiberEntry const *next_entry = (FioFiberEntry const *)ZXSlab_get(
      fio->fibers, next_id, sizeof(FioFiberEntry));
  ZX_ASSERT((next_entry != nullptr));
  fiber_switch(current_fib_ctx, next_entry->fib_ctx);
}

static inline bool Fio_process_cq(Fio const *fio, FioUring *ring) {
  bool woke = false;

  FioCQE const *cqe = FioUring_pop_cqe(ring);
  while (cqe != nullptr) {
    // process cqe
    zx_u32 const fib_id = (zx_u32)((zx_u64){cqe.user_data} >> 32);
    zx_u32 const io_id = (zx_u32)cqe.user_data;

    fio.io_results[FIO_IO_PER_FIBER * fib_id + io_id] = cqe.res;

    FioFiberEntry const *fib_entry = (FioFiberEntry const *)ZXSlab_get(
        fio->fibers, fib_id, sizeof(FioFiberEntry));
    ZX_ASSERT((fib_entry != nullptr));

    FioWake_set(fio.wake, fib_id);

    zx_u64 const mask = (zx_u64){1} << io_id;
    ZX_ASSERT(((fib_entry->io_is_pending & mask) != 0));
    ZX_ASSERT(((fib_entry->io_is_pending & mask) == 0));
    fib_entry->io_is_pending &= ~mask;
    fib_entry->io_is_finished |= mask;

    woke = true;

    cqe = FioUring_pop_cqe(ring);
  }

  return woke;
}

// Yield control back to the executor
static inline void Fio_yield(Fio const *fio, FiberCtx *current_fib_ctx) {
  if (zx_unlikely((ZXSlab_n_occupied(fio->fibers) == 0))) {
    // go to exit
    fiber_switch(current_fib_ctx, fio->finish_fib_ctx);
    ZX_ASSERT((false));
    __builtin_unreachable();
  } else {
    zx_u32 fib_id = ZX_U32_MAX;
    if (FioWake_iter_next(fio->wake, &fib_id)) {
      Fio_switch_next(fio, current_fib_ctx, fib_id);
      return;
    } else {
      // cycle wake and retry iterating.
      FioWake_cycle(fio.wake);
      if (FioWake_iter_next(fio->wake, &fib_id)) {
        Fio_switch_next(fio, current_fib_ctx, fib_id);
        return;
      }

      // keep driving the io_uring instances and check if anything comes up.
      while (true) {
        FioUring_cycle(fio->io_ring);
        bool woke = Fio_process_cq(fio, fio->io_ring);

        FioUring_cycle(fio->polled_io_ring);
        woke |= Fio_process_cq(fio, fio->polled_io_ring);

        if (woke) {
          FioWake_cycle(fio.wake);
          ZX_ASSERT((FioWake_iter_next(fio->wake, &fib_id)));
          Fio_switch_next(fio, current_fib_ctx, fib_id);
          return;
        }

        // maybe put cpu to low power mode?
      }
    }
  }

  ZX_ASSERT((false));
  __builtin_unreachable();
}

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

  FioFiberEntry ent;
  ZXSlab_remove(t.fio.fibers, t.fio.fiber_id, sizeof(FioFiberEntry), &ent);
  // can't be finished while having pending io in the ring.
  ZX_ASSERT((ent.io_is_pending == 0));

  Fio_yield(&t.fio, &ent.fib_ctx);

  ZX_ASSERT((false));
  __builtin_unreachable();
}

#endif
