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
} Ctx;

static void __attribute__((noreturn))
myfib(FiberCtx const *parent_ctx, FiberCtx *fiber_ctx, void *usr_ctx) {
  Ctx *ctxp = (Ctx *)usr_ctx;
  Ctx const ctx = *ctxp;

  ShinErrStack *err_stack = ctx.err_stack;
  ShinAlloc *alloc = ctx.alloc;

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
      if (i % 32 == 0) {
        fiber_switch(fiber_ctx, parent_ctx);
      }
    }

    shin_f64 const hash_ticks_per_elem =
        (shin_f64)ShinTimer_lap(&timer) / (shin_f64)CAP;
    printf("Hashing: %.3f ticks per element\n", (double){hash_ticks_per_elem});

    for (shin_usize i = 0; i < CAP; i++) {
      key_type const key = i;
      ShinHashtable_insert(&table, hashes[i], &key, sizeof(key_type), nullptr,
                           keys_eql);
      if (i % 32 == 0) {
        fiber_switch(fiber_ctx, parent_ctx);
      }
    }

    shin_f64 const insert_ticks_per_elem =
        (shin_f64)ShinTimer_lap(&timer) / (shin_f64)CAP;
    printf("Insert: %.3f ticks per element\n", (double){insert_ticks_per_elem});

    for (shin_usize i = 0; i < CAP; i++) {
      key_type const key = i;
      SHIN_ASSERT(
          (nullptr != ShinHashtable_get(&table, hashes[i], &key,
                                        sizeof(key_type), nullptr, keys_eql)));
      if (i % 32 == 0) {
        fiber_switch(fiber_ctx, parent_ctx);
      }
    }

    shin_f64 const get_ticks_per_elem =
        (shin_f64)ShinTimer_lap(&timer) / (shin_f64)CAP;
    printf("Get: %.3f ticks per element\n", (double){get_ticks_per_elem});
  }

  *ctx.finished = true;
  fiber_switch(fiber_ctx, parent_ctx);

  // don't implicitly return from this function
  SHIN_ASSERT((false));
}

constexpr shin_usize STACK_SIZE = 512 * 1024;

ShinStatus dobench(ShinAlloc *alloc, ShinErrStack *err_stack) {
  void *fiber_stack_alloc = nullptr;
  SHIN_ERR_TRY(
      (ShinAlloc_aligned_alloc(alloc, FIBER_STACK_ALIGN,
                               FIBER_STACK_RED_ZONE_SIZE + STACK_SIZE,
                               (void **)&fiber_stack_alloc, err_stack)),
      err_stack);

  void *fiber_stack = &fiber_stack_alloc[STACK_SIZE];

  // these will be filled by fiber_start call below
  FiberCtx parent_ctx = {0};
  FiberCtx fib_ctx = {0};

  bool finished = false;

  Ctx ctx = {
      .finished = &finished,
      .err_stack = err_stack,
      .alloc = alloc,
  };

  fiber_start(&parent_ctx, &fib_ctx, (void *)&ctx, myfib, fiber_stack);
  while (!finished) {
    fiber_switch(&parent_ctx, &fib_ctx);
  }

  return ShinStatus_Ok;
}

int main(void) {
  void *const page = shin_page_alloc_thp_2mb(MEMORY_SIZE);

  ShinErrStack err_stack = ShinErrStack_init();
  ShinAlloc alloc = ShinAlloc_init(page, MEMORY_SIZE);

  SHIN_ERR_TRACE_ABORT((dobench(&alloc, &err_stack)), &err_stack);

  shin_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}
