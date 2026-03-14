#include <fiber/fiber.h>
#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fmt.h>
#include <zx/zx_hashtable.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_rapidhash_v3.h>
#include <zx/zx_sys.h>
#include <zx/zx_time.h>
#include <zx/zx_xoshiro256.h>

constexpr zx_u64 MEMORY_SIZE = 1 << 30;
constexpr zx_u64 CAP = 40000;

constexpr zx_u64 N_FIBERS = 20;

typedef zx_u256 key_type;

static inline bool keys_eql(void const *ctx, void const *a, void const *b) {
  (void)ctx;
  key_type const *ap = (key_type const *)a;
  key_type const *bp = (key_type const *)b;
  return *ap == *bp;
}

typedef struct Ctx {
  ZXAlloc *alloc;
  ZXErrStack *err_stack;
  ZXFmt *fmt;
  bool *finished;
  zx_u64 switch_interval;
} Ctx;

static void __attribute__((noreturn))
myfib(FiberCtx const *parent_ctx, FiberCtx *fiber_ctx, void *usr_ctx) {
  Ctx *ctxp = (Ctx *)usr_ctx;
  Ctx const ctx = *ctxp;

  ZXErrStack *err_stack = ctx.err_stack;
  ZXAlloc *alloc = ctx.alloc;
  zx_u64 const switch_interval = ctx.switch_interval;

  ZXHashtable table;
  ZX_ERR_TRY_FATAL((ZXHashtable_init(alloc, CAP, alignof(key_type),
                                     sizeof(key_type), &table, err_stack)),
                   err_stack);

  zx_u64 *hashes = nullptr;
  ZX_ERR_TRY_FATAL((ZXAlloc_alloc(alloc, alignof(zx_u64), sizeof(zx_u64) * CAP,
                                  (zx_u8 **)&hashes, err_stack)),
                   err_stack);
  ZX_ASSERT((hashes != nullptr));

  for (zx_u64 x = 0; x < 5; x++) {
    ZXTimer timer = ZXTimer_init();

    ZXXoshiro256 rand = ZXXoshiro256_init(0);
    for (zx_u64 i = 0; i < CAP; i++) {
      key_type key = 0;
      ZXXoshiro256_fill(&rand, (zx_u8 *)&key, sizeof(key_type));

      zx_u64 const hash =
          zx_rapidhash_v3((zx_u8 const *)&key, sizeof(key_type), 0);
      hashes[i] = hash;
      if (i % switch_interval == 0) {
        fiber_switch(fiber_ctx, parent_ctx);
      }
    }

    zx_u64 const hash_ticks_per_elem = ZXTimer_lap(&timer) / (CAP * N_FIBERS);
    ZX_FMT(ctx.fmt, "Hash: ", hash_ticks_per_elem, " ticks per element.\n");

    for (zx_u64 i = 0; i < CAP; i++) {
      key_type const key = i;
      ZXHashtable_insert(&table, hashes[i], &key, sizeof(key_type), nullptr,
                         keys_eql);
      if (i % switch_interval == 0) {
        fiber_switch(fiber_ctx, parent_ctx);
      }
    }

    zx_u64 const insert_ticks_per_elem = ZXTimer_lap(&timer) / (CAP * N_FIBERS);
    ZX_FMT(ctx.fmt, "Insert: ", insert_ticks_per_elem, " ticks per element.\n");

    for (zx_u64 i = 0; i < CAP; i++) {
      key_type const key = i;
      ZX_ASSERT(
          (nullptr != ZXHashtable_get(&table, hashes[i], &key, sizeof(key_type),
                                      nullptr, keys_eql)));
      if (i % switch_interval == 0) {
        fiber_switch(fiber_ctx, parent_ctx);
      }
    }

    zx_u64 const get_ticks_per_elem = ZXTimer_lap(&timer) / (CAP * N_FIBERS);
    ZX_FMT(ctx.fmt, "Get: ", get_ticks_per_elem, " ticks per element.\n");
  }

  *ctx.finished = true;
  fiber_switch(fiber_ctx, parent_ctx);

  // don't implicitly return from this function
  ZX_ASSERT((false));
}

constexpr zx_u64 STACK_SIZE = 128 * 1024;

ZXStatus dobench(ZXAlloc *alloc, ZXErrStack *err_stack) {
  constexpr zx_u32 FMT_CAP = 1 << 13;
  char fmt_buf[FMT_CAP] = {0};
  ZXFmt fmt = ZXFmt_init(fmt_buf, FMT_CAP);

  Ctx ctxes[N_FIBERS] = {0};
  void *fiber_staxes[N_FIBERS] = {0};

  ZXXoshiro256 rand = ZXXoshiro256_init(69);

  for (zx_u64 fiber_idx = 0; fiber_idx < N_FIBERS; fiber_idx++) {
    zx_u8 *fiber_stack_alloc = nullptr;
    zx_u64 const skew = (ZXXoshiro256_next(&rand) % (N_FIBERS * 2)) * 64;
    zx_u64 const alloc_size = zx_next_multiple_of_u64(
        FIBER_STACK_RED_ZONE_SIZE + STACK_SIZE + skew, (zx_u64){64});
    ZX_ERR_TRY((ZXAlloc_alloc(alloc, 64, alloc_size,
                              (zx_u8 **)&fiber_stack_alloc, err_stack)),
               err_stack);

    fiber_staxes[fiber_idx] = &fiber_stack_alloc[STACK_SIZE];

    bool *finished = nullptr;
    ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(bool), sizeof(bool),
                              (zx_u8 **)&finished, err_stack)),
               err_stack);
    *finished = false;

    Ctx ctx = {
        .finished = finished,
        .err_stack = err_stack,
        .fmt = &fmt,
        .alloc = alloc,
        .switch_interval = (fiber_idx + 1) * 5,
    };

    ctxes[fiber_idx] = ctx;
  }

  FiberCtx fiber_ctxes[N_FIBERS] = {0};
  FiberCtx parent_ctx = {0};

  for (zx_u64 fiber_idx = 0; fiber_idx < N_FIBERS; fiber_idx++) {
    fiber_start(&parent_ctx, &fiber_ctxes[fiber_idx], (void *)&ctxes[fiber_idx],
                myfib, fiber_staxes[fiber_idx]);
  }
  bool running = true;
  while (running) {
    running = false;
    for (zx_u64 fiber_idx = 0; fiber_idx < N_FIBERS; fiber_idx++) {
      if (!(*ctxes[fiber_idx].finished)) {
        fiber_switch(&parent_ctx, &fiber_ctxes[fiber_idx]);
      }
      running |= !(*ctxes[fiber_idx].finished);
    }
  }

  zx_u64 const to_write = ZXFmt_n_written(&fmt);
  zx_u64 n_written = 0;
  ZX_ERR_TRY((zx_sys_write(ZX_STDERR, (zx_u8 const *)fmt_buf, to_write,
                           &n_written, err_stack)),
             err_stack);
  ZX_ERR_ASSERT((n_written == to_write), err_stack);

  return ZXStatus_Ok;
}

int main(void) {
  ZXErrStack err_stack = ZXErrStack_init();

  zx_u8 *mem = nullptr;
  ZX_ERR_TRY_FATAL((zx_page_alloc_huge_1gb(MEMORY_SIZE, &mem, &err_stack)),
                   &err_stack);

  ZXAlloc alloc = ZXAlloc_init(mem, MEMORY_SIZE);

  ZX_ERR_TRY_FATAL((dobench(&alloc, &err_stack)), &err_stack);

  ZX_ERR_TRY_FATAL((zx_page_alloc_free(mem, MEMORY_SIZE, &err_stack)),
                   &err_stack);
  return 0;
}
