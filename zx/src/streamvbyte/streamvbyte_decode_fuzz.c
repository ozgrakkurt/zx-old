#include <streamvbyte/streamvbyte.h>
#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_xoshiro256.h>

static ZXStatus do_decode(zx_u8 const *restrict compressed,
                          zx_u32 compressed_size, zx_u32 *restrict uncompressed,
                          zx_u32 uncompressed_len, zx_u8 *restrict compressed2,
                          ZXErrStack *err_stack) {
  ZX_ASSERT((compressed != nullptr));
  ZX_ASSERT((uncompressed != nullptr));
  ZX_ASSERT((compressed2 != nullptr));

  zx_u32 n_read = 0;
  ZXStatus const decode_status =
      streamvbyte_0124_decode(compressed, compressed_size, uncompressed,
                              uncompressed_len, &n_read, err_stack);
  // just return if data is detected to be invalid
  if (decode_status != ZXStatus_Ok) {
    return ZXStatus_Ok;
  }

  // zx_u32 compressed_size2 = 0;
  // ZX_ERR_TRY(
  //     (streamvbyte_0124_encode(uncompressed, uncompressed_len, compressed2,
  //                              compressed_size, &compressed_size2,
  //                              err_stack)),
  //     err_stack);
  //
  // ZX_ERR_ASSERT((compressed_size2 >= n_read), err_stack);

  // ZX_ERR_ASSERT((__builtin_memcmp(compressed2, compressed, n_read) == 0),
  //               err_stack);

  return ZXStatus_Ok;
}

typedef struct FuzzCtx {
  zx_u8 *compressed;
  zx_u8 *compressed2;
  zx_u32 *uncompressed;
} FuzzCtx;

constexpr zx_u32 COMPRESSED_CAP = 1 << 12;
constexpr zx_u32 UNCOMPRESSED_CAP = 1 << 10;

static void fill(ZXXoshiro256 *rand, zx_u8 *data, zx_u32 cap, zx_u8 fill_algo) {
  switch (fill_algo) {
  case 0: {
    ZXXoshiro256_fill(rand, data, cap);
    break;
  }
  case 1: {
    __builtin_memset(data, 0, cap);
    break;
  }
  case 2: {
    __builtin_memset(data, ~0, cap);
    break;
  }
  default: {
    ZX_ASSERT((false));
    break;
  }
  }
}

static ZXStatus fuzz_one(void *usr_ctx, ZXFuzzInput *fuzz_input,
                         ZXErrStack *err_stack) {
  FuzzCtx const *ctx = (FuzzCtx const *)usr_ctx;

  zx_u32 const uncompressed_len =
      (zx_u32)ZXFuzzInput_next(fuzz_input) % UNCOMPRESSED_CAP;
  zx_u32 const compressed_size =
      (zx_u32)ZXFuzzInput_next(fuzz_input) % COMPRESSED_CAP;

  zx_u32 const header_size =
      (compressed_size > 0)
          ? ((zx_u32)ZXFuzzInput_next(fuzz_input) % compressed_size)
          : 0;
  zx_u32 const body_size = compressed_size - compressed_size;

  zx_u8 const header_fill_algo = (zx_u8)(ZXFuzzInput_next(fuzz_input) % 3);
  zx_u8 const body_fill_algo = (zx_u8)(ZXFuzzInput_next(fuzz_input) % 3);

  ZXXoshiro256 rand = ZXXoshiro256_init(ZXFuzzInput_next(fuzz_input));

  fill(&rand, ctx->compressed, header_size, header_fill_algo);
  fill(&rand, &ctx->compressed[header_size], body_size, body_fill_algo);

  __builtin_memset(ctx->uncompressed, 0, sizeof(zx_u32) * UNCOMPRESSED_CAP);
  __builtin_memset(ctx->compressed2, 0, COMPRESSED_CAP);

  ZX_ERR_TRY((do_decode(ctx->compressed, compressed_size, ctx->uncompressed,
                        uncompressed_len, ctx->compressed2, err_stack)),
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

  zx_u32 *f_uncompressed = nullptr;
  ZX_ERR_TRY_FATAL(ZXAlloc_alloc(&alloc, alignof(zx_u32),
                                 sizeof(zx_u32) * UNCOMPRESSED_CAP,
                                 (zx_u8 **)&f_uncompressed, &err_stack),
                   &err_stack);

  zx_u8 *f_compressed = nullptr;
  ZX_ERR_TRY_FATAL(ZXAlloc_alloc(&alloc, alignof(zx_u8), COMPRESSED_CAP,
                                 &f_compressed, &err_stack),
                   &err_stack);
  zx_u8 *f_compressed2 = nullptr;
  ZX_ERR_TRY_FATAL(ZXAlloc_alloc(&alloc, alignof(zx_u8), COMPRESSED_CAP,
                                 &f_compressed2, &err_stack),
                   &err_stack);

  __builtin_memset(f_uncompressed, 0, sizeof(zx_u32) * UNCOMPRESSED_CAP);
  __builtin_memset(f_compressed, 0, COMPRESSED_CAP);
  __builtin_memset(f_compressed2, 0, COMPRESSED_CAP);

  FuzzCtx const ctx = {
      .compressed = f_compressed,
      .compressed2 = f_compressed2,
      .uncompressed = f_uncompressed,
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
