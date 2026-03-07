#include <xorf/xorf.h>
#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_hashtable.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_rapidhash_v3.h>
#include <zx/zx_xoshiro256.h>

typedef struct FuzzCtx {
  void *mem;
  zx_u32 cap;
} FuzzCtx;

static inline bool u64_eql(void const *ctx, void const *a, void const *b) {
  (void)ctx;
  zx_u64 const *ap = (zx_u64 const *)a;
  zx_u64 const *bp = (zx_u64 const *)b;
  return *ap == *bp;
}

static ZXStatus test_one(ZXAlloc *alloc, zx_u256 const *keys, zx_u32 num_keys,
                         ZXErrStack *err_stack) {
  ZXHashtable hashtable;
  ZX_ERR_TRY((ZXHashtable_init(alloc, num_keys, alignof(zx_u64), sizeof(zx_u64),
                               &hashtable, err_stack)),
             err_stack);

  zx_u64 *hashes = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, num_keys * sizeof(zx_u64), (void **)&hashes,
                            err_stack)),
             err_stack);

  zx_u32 num_hashes = 0;
  for (zx_u32 i = 0; i < num_keys; i++) {
    zx_u256 const key = keys[i];
    zx_u64 const hash =
        zx_rapidhash_v3((zx_u8 const *)&key, sizeof(zx_u256), 0);

    bool const existed_before = ZXHashtable_insert(
        &hashtable, hash, &hash, sizeof(zx_u64), nullptr, u64_eql);

    if (!existed_before) {
      hashes[num_hashes] = hash;
      num_hashes += 1wbu;
    }
  }

  XorfHeader header = {0};
  zx_u16 *fingerprints = nullptr;
  ZX_ERR_TRY((xorf_construct(alloc, alloc, hashes, num_hashes, &header,
                             &fingerprints, err_stack)),
             err_stack);

  for (zx_usize i = 0; i < num_hashes; i++) {
    zx_u64 const hash = hashes[i];
    ZX_ERR_ASSERT((xorf_check(&header, fingerprints, hash)), err_stack);
  }

  return ZXStatus_Ok;
}

static ZXStatus fuzz_one(void *usr_ctx, ZXFuzzInput *fuzz_input,
                         ZXErrStack *err_stack) {
  constexpr zx_u32 MAX_NUM_KEYS = 4096;

  FuzzCtx const *ctx = (FuzzCtx const *)usr_ctx;
  ZXAlloc alloc = ZXAlloc_init(ctx->mem, ctx->cap);

  zx_u32 const num_keys = (zx_u32)(ZXFuzzInput_next(fuzz_input) % MAX_NUM_KEYS);

  zx_u8 const distribution = (zx_u8)(ZXFuzzInput_next(fuzz_input) % 3);

  zx_u256 *keys = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(&alloc, sizeof(zx_u256) * num_keys, (void **)&keys,
                            err_stack)),
             err_stack);

  switch (distribution) {
  case 0: {
    __builtin_memset(keys, 0, sizeof(zx_u256) * num_keys);
    break;
  }
  case 1: {
    __builtin_memset(keys, ~0, sizeof(zx_u256) * num_keys);
    break;
  }
  case 2: {
    ZXXoshiro256 rand = ZXXoshiro256_init(0);
    ZXXoshiro256_fill(&rand, (zx_u8 *)keys, sizeof(zx_u256) * num_keys);
    break;
  }
  default: {
    ZX_ASSERT((false));
    break;
  }
  }

  ZX_ERR_TRY((test_one(&alloc, keys, num_keys, err_stack)), err_stack);

  return ZXStatus_Ok;
}

int main(void) {
  constexpr zx_usize MEM_CAP = (zx_usize){1} << 30;
  constexpr zx_usize FUZZ_MEM_CAP = (zx_usize){1} << 29;

  void *mem = zx_page_alloc_thp_2mb(MEM_CAP);
  ZXAlloc alloc = ZXAlloc_init(mem, MEM_CAP);

  ZXErrStack err_stack = ZXErrStack_init();

  void *fuzz_mem = nullptr;
  ZX_ERR_TRACE_TRY_ABORT(
      ZXAlloc_alloc(&alloc, FUZZ_MEM_CAP, &fuzz_mem, &err_stack), &err_stack);

  FuzzCtx const ctx = {
      .mem = fuzz_mem,
      .cap = FUZZ_MEM_CAP,
  };

  char const *tgt_file_filter = "xorf";

  ZXFuzzParams const params = {
      .usr_ctx = (void *)&ctx,
      .fuzz_one = fuzz_one,
      .tgt_file_filter = tgt_file_filter,
      .tgt_file_filter_len = __builtin_strlen(tgt_file_filter),
  };

  // initiate main loop
  ZX_ERR_TRACE_TRY_ABORT((zx_fuzz_main(&alloc, &params, &err_stack)),
                         &err_stack);

  zx_page_alloc_free(mem, MEM_CAP);
  return ZX_SYS_EXIT_SUCCESS;
}
