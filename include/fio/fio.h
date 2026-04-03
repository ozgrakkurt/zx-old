#ifndef FIO_H
#define FIO_H

#include <fiber/fiber.h>
#include <fio/fio_uring.h>
#include <fio/fio_wake.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>
#include <zx/zx_slab.h>
#include <zx/zx_time.h>

constexpr zx_u64 FIO_STACK_CANARY = 0xe3d2df15226c69d1;

// Entry is used by the executor to keep info about a Fiber.
// Entry is also used by both the executor and the fiber itself for
// communicating with each other.
struct FioFiberEntry {
  FiberCtx fib_ctx;
  // This is a pointer to a 64bit value that should be equal to
  // `FIO_STACK_CANARY`. This is intended to be used for detecting stack
  // overflow. The executor will check if this value changed every time a fiber
  // yields to detect stack overflow and it will crash the program if that is
  // the case.
  zx_u8 *stack_canary;
  zx_u64 io_is_finished;
  zx_u64 io_is_pending;
};

// has to be same as bitsize of u64 because we use u64 bitmaps for recording io
// status.
constexpr zx_u8 FIO_IO_PER_FIBER = sizeof(zx_u64) * 8;

// Copied into fiber stack at statup of a fiber and re-used by the running
// fiber.
struct Fio {
  // ---- Global ----
  FioUring *io_ring;
  FioUring *polled_io_ring;
  zx_i32 *io_results;
  FioWake *wake;
  FiberCtx const *finish_fib_ctx;
  zx_u64 yield_after_n_ticks;
  ZXSlab<FioFiberEntry> *fibers;
  ZXTimer *switch_timer;
  // ---- Fiber specific ----
  FioFiberEntry *self_entry;
  zx_u32 self_id;

  // It is illegal to call this function when a future already has max number of
  // pending/finished io.
  // Returns a pointer to sqe entry and marks it as initialized if an empty
  // entry can be found at this time. Returned entry should be immediately
  // initialized after calling this function. Returns nullptr if an entry can't
  // be found. Caller should retry later.
  inline FioSQE *push_sqe_in_place_(bool polled_io) const {
    FioFiberEntry *entry = this->self_entry;

    zx_u64 const is_filled = entry->is_pending | entry->is_finished;
    ZX_ASSERT((is_filled != ZX_U64_MAX));

    // bit index of the first zero
    zx_u8 const io_id = zx_chk_sub_u8(zx_ctz_u64(~is_filled), 1);
    zx_u64 const mask = zx_chk_shl_u64(1, io_id);

    FioSQE *sqe = nullptr;
    if (polled_io) {
      sqe = this->polled_io_ring.push_sqe_in_place();
    } else {
      sqe = this->io_ring.push_sqe_in_place();
    }
    if (sqe == nullptr) {
      return nullptr;
    }

    sqe->user_data = ((zx_u64){this->self_id} << 32) | (zx_u64){io_id};

    // mark it pending
    entry->is_pending |= mask;

    return sqe;
  }

  inline bool remove_io_result_(zx_u32 io_id, zx_i32 *result) const {
    FioFibEntry *entry = this->self_entry;

    zx_u64 const mask = (zx_u64){1} << io_id;

    if ((entry->is_finished & mask) != 0) {
      // remove finished marker
      entry->is_finished &= ~mask;
      *result = this->io_results[this->self_id * FIO_IO_PER_FUTURE + io_id];
      return true;
    } else {
      return false;
    }
  }

  inline void switch_to_(zx_u32 next_id) const {
    FioFiberEntry const *next_entry = this->fibers->get(next_id);
    ZX_ASSERT((next_entry != nullptr));
    this->switch_timer->lap();
    fiber_switch(&this->self_entry->fib_ctx, &next_entry->fib_ctx);
  }

  inline bool process_cq_(FioUring *ring_) const {
    bool woke = false;

    FioCQE const *cqe = ring_->pop_cqe_in_place();
    while (cqe != nullptr) {
      // process cqe
      zx_u32 const fib_id = (zx_u32)((zx_u64){cqe.user_data} >> 32);
      zx_u32 const io_id = (zx_u32)cqe.user_data;

      this->io_results[FIO_IO_PER_FIBER * fib_id + io_id] = cqe.res;

      FioFiberEntry const *fib_entry = this->fibers->get(fib_id);
      ZX_ASSERT((fib_entry != nullptr));

      this->wake->set(fib_id);

      zx_u64 const mask = zx_chk_shl_u64(1, io_id);
      ZX_ASSERT(((fib_entry->io_is_pending & mask) != 0));
      ZX_ASSERT(((fib_entry->io_is_finished & mask) == 0));
      fib_entry->io_is_pending &= ~mask;
      fib_entry->io_is_finished |= mask;

      woke = true;

      cqe = ring_->pop_cqe_in_place();
    }

    return woke;
  }

  // Yield control back to the executor
  inline void yield_(FiberCtx *current_fib_ctx) const {
    if (this->fibers->n_occupied() == 0) [[unlikely]] {
      // go to exit
      this->switch_timer->lap();
      fiber_switch(current_fib_ctx, this->finish_fib_ctx);
      ZX_ASSERT((false));
      __builtin_unreachable();
    } else {
      zx_u32 fib_id = ZX_U32_MAX;
      if (this->wake->iter_next(&fib_id) {
        this->switch_to_(fib_id);
        return;
      } else {
        // cycle wake and retry iterating.
        this->wake->cycle();
        if (this->wake->iter_next(&fib_id)) {
          this->switch_to_(fib_id);
          return;
        }

        // Keep driving the io_uring instances and check if anything comes up.
        while (true) {
          this->io_ring.cycle();
          bool woke = this->process_cq_(this->io_ring);

          this->polled_io_ring.cycle();
          woke |= this->process_cq_(this->polled_io_ring);

          if (woke) {
            this->wake->cycle();
            ZX_ASSERT((this->wake->iter_next(&fib_id)));
            this->switch_to_(fib_id);
            return;
          }

          // Make cpu relax since we are busy looping while we wait io to
          // finish. Emits x86_64 `PAUSE` instruction.
          __builtin_ia32_pause();
        }
      }
    }

    ZX_ASSERT((false));
    __builtin_unreachable();
  }

  inline void yield_if_needed_() const {
    if (this->switch_timer->elapsed() > this->yield_after_n_ticks) {
      this->yield_();
    }
  }
}; // struct Fio

static inline void fio_yield_if_needed(Fio const *fio) {
  fio.yield_if_needed_();
}

#endif
