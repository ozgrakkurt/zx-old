#include <noz/noz.h>
#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_xoshiro256.h>

static ZXStatus roundtrip(zx_u8 const *__restrict input,
                          zx_u8 *__restrict compressed,
                          zx_u8 *__restrict output, zx_u64 len,
                          ZXErrStack *err_stack) {
  ZX_ASSERT((input != nullptr));
  ZX_ASSERT((compressed != nullptr));
  ZX_ASSERT((output != nullptr));

  zx_u64 const compress_bound = noz_compress_bound(len);

  zx_u64 const compressed_size =
      noz_compress(input, len, compressed, compress_bound);
  ZX_ERR_ASSERT((compressed_size <= compress_bound), err_stack);

  zx_u64 const n_read = noz_decompress(compressed, output, len);
  ZX_ERR_ASSERT((n_read == compressed_size), err_stack);

  ZX_ERR_ASSERT((__builtin_memcmp(input, output, len) == 0), err_stack);

  return ZXStatus::Ok;
}

typedef struct FuzzCtx {
  zx_u8 *input;
  zx_u8 *compressed;
  zx_u8 *output;
  zx_u64 compressed_cap;
} FuzzCtx;

constexpr zx_u32 IN_OUT_CAP = 1 << 12;

static ZXStatus fuzz_one(void *usr_ctx, ZXFuzzInput *fuzz_input,
                         ZXErrStack *err_stack) {
  FuzzCtx const *ctx = (FuzzCtx const *)usr_ctx;

  zx_u64 const len = fuzz_input->next() % IN_OUT_CAP;

  zx_u64 const fill_algo = fuzz_input->next() % 4;

  ZXXoshiro256 rand = ZXXoshiro256::init(fuzz_input->next());

  switch (fill_algo) {
  case 0: {
    for (zx_u32 i = 0; i < len; i++) {
      ctx->input[i] = ((rand.next() & 3) == 0) ? (zx_u8)rand.next() : 0;
    }
    break;
  }
  case 1: {
    rand.fill(ctx->input, len);
    break;
  }
  case 2: {
    __builtin_memset(ctx->input, 0, len);
    break;
  }
  case 3: {
    __builtin_memset(ctx->input, ~0, len);
    break;
  }
  default: {
    ZX_ASSERT((false));
    break;
  }
  }

  __builtin_memset(ctx->output, 0, IN_OUT_CAP);
  __builtin_memset(ctx->compressed, 0, ctx->compressed_cap);

  ZX_ERR_TRY(
      (roundtrip(ctx->input, ctx->compressed, ctx->output, len, err_stack)),
      err_stack);

  return ZXStatus::Ok;
}

int main(void) {
  constexpr zx_u64 MEM_CAP = (zx_u64){1} << 30;

  ZXErrStack err_stack = ZXErrStack::init();

  zx_u8 *mem = nullptr;
  ZX_ERR_TRY_FATAL((zx_page_alloc_thp_2mb(MEM_CAP, &mem, &err_stack)),
                   &err_stack);

  ZXAlloc alloc = ZXAlloc::init(mem, MEM_CAP);

  zx_u8 *f_input = nullptr;
  ZX_ERR_TRY_FATAL(alloc.alloc<zx_u8>(IN_OUT_CAP, &f_input, &err_stack),
                   &err_stack);
  zx_u8 *f_output = nullptr;
  ZX_ERR_TRY_FATAL(alloc.alloc<zx_u8>(IN_OUT_CAP, &f_output, &err_stack),
                   &err_stack);

  zx_u64 const f_compr_cap = noz_compress_bound(IN_OUT_CAP);

  zx_u8 *f_compressed = nullptr;
  ZX_ERR_TRY_FATAL(alloc.alloc<zx_u8>(f_compr_cap, &f_compressed, &err_stack),
                   &err_stack);

  __builtin_memset(f_input, 0, IN_OUT_CAP);
  __builtin_memset(f_output, 0, IN_OUT_CAP);
  __builtin_memset(f_compressed, 0, f_compr_cap);

  FuzzCtx const ctx = {
      .input = f_input,
      .compressed = f_compressed,
      .output = f_output,
      .compressed_cap = f_compr_cap,
  };

  char const *tgt_file_filter = "noz";

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
