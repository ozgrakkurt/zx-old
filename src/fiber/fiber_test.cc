#include <fiber/fiber.h>
#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_hashtable.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_rapidhash_v3.h>
#include <zx/zx_xoshiro256.h>

constexpr zx_u64 STEP = 100;

static inline bool keys_eql(void const *ctx, zx_u64 const *a, zx_u64 const *b) {
  (void)ctx;
  return *a == *b;
}

using Hashtable = ZXHashtable<zx_u64, void const *, keys_eql>;

static inline zx_u64 make_hash(zx_u64 key) {
  return zx_rapidhash_v3((zx_u8 const *)&key, sizeof(zx_u64), 0);
}

struct Ctx {
  Hashtable *htable;
  zx_u64 iter_no;
};

extern "C" {
static void __attribute__((noreturn))
myfib(FiberCtx const *parent_ctx, FiberCtx *fiber_ctx, void *usr_ctx) {
  Ctx *ctxp = (Ctx *)usr_ctx;

  while (true) {
    Ctx const ctx = *ctxp;
    ZXXoshiro256 rand = ZXXoshiro256::init(ctx.iter_no);

    for (zx_u64 i = 0; i < STEP; i++) {
      zx_u64 const key = rand.next();
      zx_u64 const hash = make_hash(key);
      ZX_ASSERT((ctx.htable->insert(hash, &key, nullptr) == false));
    }

    {
      zx_u64 const key = rand.next();
      zx_u64 const hash = make_hash(key);
      ZX_ASSERT((ctx.htable->get(hash, &key, nullptr) == nullptr));
    }

    // yield to parent
    fiber_switch(fiber_ctx, parent_ctx);
  }

  // don't implicitly return from this function
  ZX_ASSERT((false));
}
}

constexpr zx_u64 STACK_SIZE = 512 * 1024;

constexpr zx_u64 NUM_ITER = STEP * 69;

ZXStatus dotest(ZXAlloc *alloc, ZXErrStack *err_stack) {
  zx_u8 *fiber_stack_alloc = nullptr;
  ZX_ERR_TRY((alloc->aligned_alloc<zx_u8>(
                 FIBER_STACK_ALIGN, FIBER_STACK_RED_ZONE_SIZE + STACK_SIZE,
                 &fiber_stack_alloc, err_stack)),
             err_stack);

  zx_u8 *fiber_stack = &fiber_stack_alloc[STACK_SIZE];

  Hashtable htable;
  ZX_ERR_TRY((Hashtable::init(alloc, NUM_ITER, &htable, err_stack)), err_stack);

  // these will be filled by fiber_start call below
  FiberCtx parent_ctx;
  FiberCtx fib_ctx;

  Ctx ctx = {
      .htable = &htable,
      .iter_no = 0,
  };

  for (zx_u64 iter_no = 0; iter_no < NUM_ITER; iter_no += STEP) {
    ctx.iter_no = iter_no;
    if (iter_no == 0) {
      fiber_start(&parent_ctx, &fib_ctx, (void *)&ctx, myfib, fiber_stack);
    } else {
      fiber_switch(&parent_ctx, &fib_ctx);
    }
    // fiber did the step yielded

    ZXXoshiro256 rand = ZXXoshiro256::init(iter_no);

    for (zx_u64 i = 0; i < STEP; i++) {
      zx_u64 const key = rand.next();
      zx_u64 const hash = make_hash(key);
      ZX_ASSERT((ctx.htable->get(hash, &key, nullptr) != nullptr));
    }

    {
      zx_u64 const key = rand.next();
      zx_u64 const hash = make_hash(key);
      ZX_ASSERT((ctx.htable->get(hash, &key, nullptr) == nullptr));
    }
  }

  return ZXStatus::Ok;
}

constexpr zx_u64 MEMORY_SIZE = 1 << 22;

int main(void) {
  ZXErrStack err_stack = ZXErrStack::init();

  zx_u8 *mem = nullptr;
  ZX_ERR_TRY_FATAL((zx_page_alloc_4kb(MEMORY_SIZE, &mem, &err_stack)),
                   &err_stack);

  ZXAlloc alloc = ZXAlloc::init(mem, MEMORY_SIZE);
  ZX_ERR_TRY_FATAL((dotest(&alloc, &err_stack)), &err_stack);

  ZX_ERR_TRY_FATAL((zx_page_alloc_free(mem, MEMORY_SIZE, &err_stack)),
                   &err_stack);
  return 0;
}
