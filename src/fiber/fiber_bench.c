#include <fiber/fiber.h>
#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <shin/shin_hashtable.h>
#include <shin/shin_page_alloc.h>
#include <shin/shin_rapidhash_v3.h>
#include <shin/shin_time.h>
#include <shin/shin_xoshiro256.h>

constexpr shin_usize MEMORY_SIZE = 1 << 30;
constexpr shin_usize CAP = 40000;

constexpr shin_usize N_FIBERS = 2;

typedef shin_u256 key_type;

static inline bool keys_eql(void const *ctx, void const *a, void const *b) {
  (void)ctx;
  key_type const *ap = (key_type const *)a;
  key_type const *bp = (key_type const *)b;
  return *ap == *bp;
}

typedef struct Ctx {
  ShinAlloc *alloc;
  ShinErrStack *err_stack;
  bool *finished;
  shin_usize switch_interval;
} Ctx;

static void __attribute__((noreturn))
myfib(FiberCtx const *parent_ctx, FiberCtx *fiber_ctx, void *usr_ctx) {
  Ctx *ctxp = (Ctx *)usr_ctx;
  Ctx const ctx = *ctxp;

  ShinErrStack *err_stack = ctx.err_stack;
  ShinAlloc *alloc = ctx.alloc;
  shin_usize const switch_interval = ctx.switch_interval;

  ShinHashtable table;
  SHIN_ERR_TRACE_ABORT(
      (ShinHashtable_init(alloc, CAP, alignof(key_type), sizeof(key_type),
                          &table, err_stack)),
      err_stack);

  shin_u64 *hashes = nullptr;
  SHIN_ERR_TRACE_ABORT((ShinAlloc_alloc(alloc, sizeof(shin_u64) * CAP,
                                        (void **)&hashes, err_stack)),
                       err_stack);
  SHIN_ASSERT((hashes != nullptr));

  for (shin_usize x = 0; x < 5; x++) {
    ShinTimer timer = ShinTimer_init();

    ShinXoshiro256 rand = ShinXoshiro256_init(0);
    for (shin_usize i = 0; i < CAP; i++) {
      key_type key = 0;
      ShinXoshiro256_fill(&rand, (shin_u8 *)&key, sizeof(key_type));

      shin_u64 const hash =
          shin_rapidhash_v3((shin_u8 const *)&key, sizeof(key_type), 0);
      hashes[i] = hash;
      if (i % switch_interval == 0) {
        fiber_switch(fiber_ctx, parent_ctx);
      }
    }

    shin_f64 const hash_ticks_per_elem =
        (shin_f64)ShinTimer_lap(&timer) / (shin_f64)(CAP * N_FIBERS);
    printf("Hashing: %.3f ticks per element\n", (double){hash_ticks_per_elem});

    for (shin_usize i = 0; i < CAP; i++) {
      key_type const key = i;
      ShinHashtable_insert(&table, hashes[i], &key, sizeof(key_type), nullptr,
                           keys_eql);
      if (i % switch_interval == 0) {
        fiber_switch(fiber_ctx, parent_ctx);
      }
    }

    shin_f64 const insert_ticks_per_elem =
        (shin_f64)ShinTimer_lap(&timer) / (shin_f64)(CAP * N_FIBERS);
    printf("Insert: %.3f ticks per element\n", (double){insert_ticks_per_elem});

    for (shin_usize i = 0; i < CAP; i++) {
      key_type const key = i;
      SHIN_ASSERT(
          (nullptr != ShinHashtable_get(&table, hashes[i], &key,
                                        sizeof(key_type), nullptr, keys_eql)));
      if (i % switch_interval == 0) {
        fiber_switch(fiber_ctx, parent_ctx);
      }
    }

    shin_f64 const get_ticks_per_elem =
        (shin_f64)ShinTimer_lap(&timer) / (shin_f64)(CAP * N_FIBERS);
    printf("Get: %.3f ticks per element\n", (double){get_ticks_per_elem});
  }

  *ctx.finished = true;
  fiber_switch(fiber_ctx, parent_ctx);

  // don't implicitly return from this function
  SHIN_ASSERT((false));
}

constexpr shin_usize STACK_SIZE = 128 * 1024;

ShinStatus dobench(ShinAlloc *alloc, ShinErrStack *err_stack) {
  Ctx ctxes[N_FIBERS] = {0};
  void *fiber_staxes[N_FIBERS] = {0};

  ShinXoshiro256 rand = ShinXoshiro256_init(69);

  for (shin_usize fiber_idx = 0; fiber_idx < N_FIBERS; fiber_idx++) {
    void *fiber_stack_alloc = nullptr;
    shin_usize const skew = (ShinXoshiro256_next(&rand) % (N_FIBERS * 2)) * 64;
    shin_usize const alloc_size = SHIN_NEXT_MULTIPLE_OF(
        FIBER_STACK_RED_ZONE_SIZE + STACK_SIZE + skew, (shin_usize){64});
    SHIN_ERR_TRY(
        (ShinAlloc_aligned_alloc(alloc, 64, alloc_size,
                                 (void **)&fiber_stack_alloc, err_stack)),
        err_stack);

    fiber_staxes[fiber_idx] = &fiber_stack_alloc[STACK_SIZE];

    bool *finished = nullptr;
    SHIN_ERR_TRY(
        (ShinAlloc_alloc(alloc, sizeof(bool), (void **)&finished, err_stack)),
        err_stack);
    *finished = false;

    Ctx ctx = {
        .finished = finished,
        .err_stack = err_stack,
        .alloc = alloc,
        .switch_interval = (fiber_idx + 1) * 5,
    };

    ctxes[fiber_idx] = ctx;
  }

  FiberCtx fiber_ctxes[N_FIBERS] = {0};
  FiberCtx parent_ctx = {0};

  for (shin_usize fiber_idx = 0; fiber_idx < N_FIBERS; fiber_idx++) {
    fiber_start(&parent_ctx, &fiber_ctxes[fiber_idx], (void *)&ctxes[fiber_idx],
                myfib, fiber_staxes[fiber_idx]);
  }
  bool running = true;
  while (running) {
    running = false;
    for (shin_usize fiber_idx = 0; fiber_idx < N_FIBERS; fiber_idx++) {
      if (!(*ctxes[fiber_idx].finished)) {
        fiber_switch(&parent_ctx, &fiber_ctxes[fiber_idx]);
      }
      running |= !(ctxes[fiber_idx].finished);
    }
  }

  return ShinStatus_Ok;
}

int main(void) {
  void *const page = shin_page_alloc_huge_1gb(MEMORY_SIZE);

  ShinErrStack err_stack = ShinErrStack_init();
  ShinAlloc alloc = ShinAlloc_init(page, MEMORY_SIZE);

  SHIN_ERR_TRACE_ABORT((dobench(&alloc, &err_stack)), &err_stack);

  shin_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}
