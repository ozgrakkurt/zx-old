#include <streamvbyte/streamvbyte.h>
#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_xoshiro256.h>

static ZXStatus roundtrip(zx_u32 const *restrict input,
                          zx_u8 *restrict compressed, zx_u32 *restrict output,
                          zx_u32 len, ZXErrStack *err_stack) {
  ZX_ASSERT((input != nullptr));
  ZX_ASSERT((compressed != nullptr));
  ZX_ASSERT((output != nullptr));

  zx_u32 compress_bound = 0;
  ZX_ERR_TRY((streamvbyte_compress_bound(len, &compress_bound, err_stack)),
             err_stack);

  zx_u32 compressed_size = 0;
  ZX_ERR_TRY((streamvbyte_0124_encode(input, len, compressed, compress_bound,
                                      &compressed_size, err_stack)),
             err_stack);

  ZX_ERR_ASSERT((compressed_size <= compress_bound), err_stack);

  zx_u32 n_read = 0;
  ZX_ERR_TRY((streamvbyte_0124_decode(compressed, compressed_size, output, len,
                                      &n_read, err_stack)),
             err_stack);

  ZX_ERR_ASSERT((n_read <= compressed_size), err_stack);

  ZX_ERR_ASSERT((__builtin_memcmp(input, output, sizeof(zx_u32) * len) == 0),
                err_stack);

  return ZXStatus_Ok;
}

typedef struct FuzzCtx {
  zx_u32 *input;
  zx_u8 *compressed;
  zx_u32 *output;
  zx_u32 compressed_cap;
} FuzzCtx;

constexpr zx_u32 IN_OUT_CAP = 1 << 10;

static ZXStatus fuzz_one(void *usr_ctx, ZXFuzzInput *fuzz_input,
                         ZXErrStack *err_stack) {
  FuzzCtx const *ctx = (FuzzCtx const *)usr_ctx;

  zx_u32 const len = (zx_u32)ZXFuzzInput_next(fuzz_input) % IN_OUT_CAP;

  zx_u64 const fill_algo = ZXFuzzInput_next(fuzz_input) % 5;

  ZXXoshiro256 rand = ZXXoshiro256_init(ZXFuzzInput_next(fuzz_input));

  zx_u32 const int_bound = (zx_u32)ZXFuzzInput_next(fuzz_input) | (zx_u32){1};

  switch (fill_algo) {
  case 0: {
    for (zx_u32 i = 0; i < len; i++) {
      ctx->input[i] =
          zx_rand_mod_u32((zx_u32)ZXXoshiro256_next(&rand), int_bound);
    }
    break;
  }
  case 1: {
    ZXXoshiro256_fill(&rand, (zx_u8 *)ctx->input, sizeof(zx_u32) * len);
    break;
  }
  case 2: {
    __builtin_memset(ctx->input, 0, sizeof(zx_u32) * len);
    break;
  }
  case 3: {
    __builtin_memset(ctx->input, ~0, sizeof(zx_u32) * len);
    break;
  }
  case 4: {
    for (zx_u32 i = 0; i < len; i++) {
      ctx->input[i] =
          zx_rand_mod_u32((zx_u32)ZXXoshiro256_next(&rand), ZX_U8_MAX);
    }
    break;
  }
  default: {
    ZX_ASSERT((false));
    break;
  }
  }

  __builtin_memset(ctx->output, 0, sizeof(zx_u32) * IN_OUT_CAP);
  __builtin_memset(ctx->compressed, 0, sizeof(zx_u8) * ctx->compressed_cap);

  ZX_ERR_TRY(
      (roundtrip(ctx->input, ctx->compressed, ctx->output, len, err_stack)),
      err_stack);

  return ZXStatus_Ok;
}

int main(void) {
  constexpr zx_u64 MEM_CAP = (zx_u64){1} << 30;

  ZXErrStack err_stack = ZXErrStack_init();

  zx_u8 *mem = nullptr;
  ZX_ERR_TRY_FATAL((zx_page_alloc_thp_2mb(MEM_CAP, &mem, &err_stack)),
                   &err_stack);

  ZXAlloc alloc = ZXAlloc_init(mem, MEM_CAP);

  zx_u32 *f_input = nullptr;
  ZX_ERR_TRY_FATAL(ZXAlloc_alloc(&alloc, alignof(zx_u32),
                                 sizeof(zx_u32) * IN_OUT_CAP,
                                 (zx_u8 **)&f_input, &err_stack),
                   &err_stack);
  zx_u32 *f_output = nullptr;
  ZX_ERR_TRY_FATAL(ZXAlloc_alloc(&alloc, alignof(zx_u32),
                                 sizeof(zx_u32) * IN_OUT_CAP,
                                 (zx_u8 **)&f_output, &err_stack),
                   &err_stack);

  zx_u32 f_compr_cap = 0;
  ZX_ERR_TRY_FATAL(
      (streamvbyte_compress_bound(IN_OUT_CAP, &f_compr_cap, &err_stack)),
      &err_stack);

  zx_u8 *f_compressed = nullptr;
  ZX_ERR_TRY_FATAL(ZXAlloc_alloc(&alloc, alignof(zx_u8),
                                 sizeof(zx_u8) * f_compr_cap, &f_compressed,
                                 &err_stack),
                   &err_stack);

  __builtin_memset(f_input, 0, sizeof(zx_u32) * IN_OUT_CAP);
  __builtin_memset(f_output, 0, sizeof(zx_u32) * IN_OUT_CAP);
  __builtin_memset(f_compressed, 0, sizeof(zx_u8) * f_compr_cap);

  FuzzCtx const ctx = {
      .input = f_input,
      .output = f_output,
      .compressed = f_compressed,
      .compressed_cap = f_compr_cap,
  };

  char const *tgt_file_filter = "streamvbyte";

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
