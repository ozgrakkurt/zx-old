#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_rapidhash_v3.h>
#include <zx/zx_slab.h>

typedef struct FuzzCtx {
  void *mem;
  zx_u32 cap;
} FuzzCtx;

static ZXStatus fuzz_one(void *usr_ctx, ZXFuzzInput *fuzz_input,
                         ZXErrStack *err_stack) {
  FuzzCtx const *ctx = (FuzzCtx const *)usr_ctx;
  ZXAlloc alloc = ZXAlloc_init(ctx->mem, ctx->cap);

  zx_u32 const MAX_CAP = 256;
  zx_u32 const capacity =
      (((zx_u32)ZXFuzzInput_next(fuzz_input)) % MAX_CAP) | 1;

  zx_u32 const MAX_N_OPS = 1024;
  zx_u32 const n_ops = ((zx_u32)ZXFuzzInput_next(fuzz_input)) % MAX_N_OPS;

  ZXSlab slab;
  ZX_ERR_TRY((ZXSlab_init(&alloc, capacity, alignof(zx_u64), sizeof(zx_u64),
                          &slab, err_stack)),
             err_stack);

  // use for randomly selecting slab keys for ops
  zx_u64 *slab_keys = nullptr;
  zx_u32 const slab_keys_byte_size = zx_chk_mul_u32(capacity, sizeof(zx_u64));
  ZX_ERR_TRY((ZXAlloc_alloc(&alloc, alignof(zx_u64), slab_keys_byte_size,
                            (zx_u8 **)&slab_keys, err_stack)),
             err_stack);
  zx_u32 n_slab_keys = 0;

  for (zx_u32 i = 0; i < n_ops; i++) {
    ZX_ERR_ASSERT(
        (zx_chk_sub_u32(capacity, ZXSlab_capacity_left(&slab)) == n_slab_keys),
        err_stack);

    zx_u8 op = 0;
    zx_u8 const op_raw = (zx_u8)ZXFuzzInput_next(fuzz_input);
    if (ZXSlab_capacity_left(&slab) == 0) {
      op = op_raw % 2; // can only do get or remove if it is full
    } else if (n_slab_keys == 0) {
      op = 2; // can only do insert if it is empty
    } else {
      op = op_raw % 3; // can do anything if it is not full or empty
    }

    switch (op) {
    // get
    case 0: {
      zx_u64 const slab_key = slab_keys[zx_rand_mod_u32(
          (zx_u32)ZXFuzzInput_next(fuzz_input), n_slab_keys)];
      zx_u64 const slab_val =
          *(zx_u64 const *)ZXSlab_get(&slab, slab_key, sizeof(zx_u64));

      ZX_ERR_ASSERT((slab_key == slab_val), err_stack);

      break;
    }
    // remove
    case 1: {
      zx_u32 const slab_key_idx =
          zx_rand_mod_u32((zx_u32)ZXFuzzInput_next(fuzz_input), n_slab_keys);
      zx_u64 const slab_key = slab_keys[slab_key_idx];
      n_slab_keys--;
      slab_keys[slab_key_idx] = slab_keys[n_slab_keys];

      zx_u64 slab_val = 0;
      ZXSlab_remove(&slab, slab_key, sizeof(zx_u64), &slab_val);

      ZX_ERR_ASSERT((slab_key == slab_val), err_stack);

      break;
    }
    // insert
    case 2: {
      zx_u64 const slab_key =
          ZXSlab_insert(&slab, &(zx_u64){0}, sizeof(zx_u64));
      *(zx_u64 *)ZXSlab_get_mut(&slab, slab_key, sizeof(zx_u64)) = slab_key;

      ZX_ASSERT((n_slab_keys < capacity));
      slab_keys[n_slab_keys] = slab_key;
      n_slab_keys++;

      break;
    }
    default: {
      ZX_ASSERT((false));
      break;
    }
    }
  }

  return ZXStatus_Ok;
}

int main(void) {
  constexpr zx_u64 MEM_CAP = (zx_u64){1} << 30;
  constexpr zx_u64 FUZZ_MEM_CAP = (zx_u64){1} << 29;

  ZXErrStack err_stack = ZXErrStack_init();

  zx_u8 *mem = nullptr;
  ZX_ERR_TRY_FATAL((zx_page_alloc_thp_2mb(MEM_CAP, &mem, &err_stack)),
                   &err_stack);

  ZXAlloc alloc = ZXAlloc_init(mem, MEM_CAP);

  zx_u8 *fuzz_mem = nullptr;
  ZX_ERR_TRY_FATAL(ZXAlloc_alloc(&alloc, alignof(zx_u8), FUZZ_MEM_CAP,
                                 &fuzz_mem, &err_stack),
                   &err_stack);

  FuzzCtx const ctx = {
      .mem = fuzz_mem,
      .cap = FUZZ_MEM_CAP,
  };

  char const *tgt_file_filter = "zx_slab";

  ZXFuzzParams const params = {
      .usr_ctx = (void *)&ctx,
      .fuzz_one = fuzz_one,
      .tgt_file_filter = tgt_file_filter,
      .tgt_file_filter_len = (zx_u16)__builtin_strlen(tgt_file_filter),
  };

  // initiate main loop
  ZX_ERR_TRY_FATAL((zx_fuzz_main(&alloc, &params, &err_stack)), &err_stack);

  ZX_ERR_TRY_FATAL((zx_page_alloc_free(mem, MEM_CAP, &err_stack)), &err_stack);

  return 0;
}
