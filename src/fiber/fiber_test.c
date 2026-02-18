#include <fiber/fiber.h>
#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <shin/shin_hashtable.h>
#include <shin/shin_page_alloc.h>
#include <shin/shin_rapidhash_v3.h>
#include <shin/shin_xoshiro256.h>

constexpr shin_usize STEP = 100;

static inline bool keys_eql(void const *ctx, void const *a, void const *b) {
  (void)ctx;
  shin_u64 const *ap = (shin_u64 const *)a;
  shin_u64 const *bp = (shin_u64 const *)b;
  return *ap == *bp;
}

static inline shin_u64 make_hash(shin_u64 key) {
  return shin_rapidhash_v3((shin_u8 const *)&key, sizeof(shin_u64), 0);
}

typedef struct Ctx {
  ShinHashtable *htable;
  shin_usize iter_no;
  FiberCtx *fib_ctx;
} Ctx;

static void __attribute__((noreturn)) myfib(FiberCtx const *prev,
                                            void *usr_ctx) {
  Ctx *ctxp = (Ctx *)usr_ctx;
  Ctx const ctx = *ctxp;

  while (true) {
    ShinXoshiro256 rand = ShinXoshiro256_init(ctx.iter_no);

    for (shin_usize i = 0; i < STEP; i++) {
      shin_u64 const key = ShinXoshiro256_next(&rand);
      shin_u64 const hash = make_hash(key);
      SHIN_ASSERT(
          (ShinHashtable_insert(ctx.htable, hash, &key, sizeof(shin_u64),
                                nullptr, keys_eql) == false));
    }

    {
      shin_u64 const key = ShinXoshiro256_next(&rand);
      shin_u64 const hash = make_hash(key);
      SHIN_ASSERT((ShinHashtable_get(ctx.htable, hash, &key, sizeof(shin_u64),
                                     nullptr, keys_eql) == nullptr));
    }

    // yield to parent
    fiber_switch(ctx.fib_ctx, prev);
  }

  // don't implicitly return from this function
  SHIN_ASSERT((false));
}

constexpr shin_usize STACK_SIZE = 512 * 1024;

constexpr shin_usize NUM_ITER = STEP * 69;

ShinStatus dotest(ShinAlloc *alloc, ShinErrStack *err_stack) {
  void *fiber_stack = nullptr;
  SHIN_ERR_TRY((ShinAlloc_aligned_alloc(alloc, FIBER_STACK_ALIGN,
                                        FIBER_STACK_RED_ZONE_SIZE + STACK_SIZE,
                                        (void **)&fiber_stack, err_stack)),
               err_stack);

  ShinHashtable htable;
  SHIN_ERR_TRY((ShinHashtable_init(alloc, NUM_ITER, alignof(shin_u64),
                                   sizeof(shin_u64), &htable, err_stack)),
               err_stack);

  // will be filled by fiber_start call below
  FiberCtx fib_ctx = {0};
  Ctx ctx = {
      .htable = &htable,
      .iter_no = 0,
      .fib_ctx = &fib_ctx,
  };

  for (shin_usize iter_no = 0; iter_no < NUM_ITER; iter_no += STEP) {
    ctx.iter_no = iter_no;
    fiber_start(ctx.fib_ctx, (void *)&ctx, myfib, fiber_stack);
    // fiber did the step yielded

    ShinXoshiro256 rand = ShinXoshiro256_init(iter_no);

    for (shin_usize i = 0; i < STEP; i++) {
      shin_u64 const key = ShinXoshiro256_next(&rand);
      shin_u64 const hash = make_hash(key);
      SHIN_ASSERT((ShinHashtable_get(ctx.htable, hash, &key, sizeof(shin_u64),
                                     nullptr, keys_eql) != nullptr));
    }

    {
      shin_u64 const key = ShinXoshiro256_next(&rand);
      shin_u64 const hash = make_hash(key);
      SHIN_ASSERT((ShinHashtable_get(ctx.htable, hash, &key, sizeof(shin_u64),
                                     nullptr, keys_eql) == nullptr));
    }
  }

  return ShinStatus_Ok;
}

constexpr shin_usize MEMORY_SIZE = 1 << 22;

int main(void) {
  void *const page = shin_page_alloc_4kb(MEMORY_SIZE);

  ShinErrStack err_stack = ShinErrStack_init();
  ShinAlloc alloc = ShinAlloc_init(page, MEMORY_SIZE);

  SHIN_ERR_TRACE_ABORT((dotest(&alloc, &err_stack)), &err_stack);

  shin_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}
