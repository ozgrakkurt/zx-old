#include <noz/noz.h>
#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>
#include <zx/zx_xoshiro256.h>

static ZXStatus
do_decode(zx_u8 const *__restrict compressed, zx_u64 compressed_size,
          zx_u8 *__restrict uncompressed, zx_u64 uncompressed_len,
          zx_u8 *__restrict compressed2, ZXErrStack *err_stack) {
  ZX_ASSERT((compressed != nullptr));
  ZX_ASSERT((uncompressed != nullptr));
  ZX_ASSERT((compressed2 != nullptr));

  zx_u64 n_read = 0;
  ZXStatus const decode_status =
      noz_decompress_safe(compressed, compressed_size, uncompressed,
                          uncompressed_len, &n_read, err_stack);
  // just return if data is detected to be invalid
  if (decode_status != ZXStatus::Ok) {
    return ZXStatus::Ok;
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

  return ZXStatus::Ok;
}

typedef struct FuzzCtx {
  zx_u8 *compressed;
  zx_u8 *compressed2;
  zx_u8 *uncompressed;
} FuzzCtx;

constexpr zx_u64 COMPRESSED_CAP = 1 << 12;
constexpr zx_u64 UNCOMPRESSED_CAP = 1 << 14;

static void fill(ZXXoshiro256 *rand, zx_u8 *data, zx_u64 cap, zx_u8 fill_algo) {
  switch (fill_algo) {
  case 0: {
    rand->fill(data, cap);
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

  zx_u64 const uncompressed_len = fuzz_input->next() % UNCOMPRESSED_CAP;
  zx_u64 const compressed_offset = fuzz_input->next() % (COMPRESSED_CAP + 1);
  zx_u64 const compressed_size = COMPRESSED_CAP - compressed_offset;
  zx_u8 *compressed = &ctx->compressed[compressed_offset];

  zx_u64 const header_size =
      (compressed_size > 0) ? (fuzz_input->next() % compressed_size) : 0;
  zx_u64 const body_size = compressed_size - header_size;

  zx_u8 const header_fill_algo = (zx_u8)(fuzz_input->next() % 3);
  zx_u8 const body_fill_algo = (zx_u8)(fuzz_input->next() % 3);

  ZXXoshiro256 rand = ZXXoshiro256::init(fuzz_input->next());

  fill(&rand, compressed, header_size, header_fill_algo);
  fill(&rand, &compressed[header_size], body_size, body_fill_algo);

  __builtin_memset(ctx->uncompressed, 0, UNCOMPRESSED_CAP);
  __builtin_memset(ctx->compressed2, 0, COMPRESSED_CAP);

  ZX_ERR_TRY((do_decode(compressed, compressed_size, ctx->uncompressed,
                        uncompressed_len, ctx->compressed2, err_stack)),
             err_stack);

  return ZXStatus::Ok;
}

int main(void) {
  constexpr zx_u64 MEM_CAP = (zx_u64){1} << 30;

  ZXErrStack err_stack = ZXErrStack::init();

  zx_u8 *mem = nullptr;
  ZX_ERR_TRY_FATAL((zx_page_alloc_thp_2mb(MEM_CAP, &mem, &err_stack)),
                   &err_stack);

  zx_u8 *f_compressed = nullptr;
  ZX_ERR_TRY_FATAL(
      (zx_page_alloc_4kb(COMPRESSED_CAP * 2, &f_compressed, &err_stack)),
      &err_stack);

  ZX_ERR_TRY_FATAL(
      (zx_sys_mprotect(&f_compressed[COMPRESSED_CAP], COMPRESSED_CAP,
                       ZX_SYS_PROT_NONE, &err_stack)),
      &err_stack);

  ZXAlloc alloc = ZXAlloc::init(mem, MEM_CAP);

  zx_u8 *f_uncompressed = nullptr;
  ZX_ERR_TRY_FATAL(
      alloc.alloc<zx_u8>(UNCOMPRESSED_CAP, &f_uncompressed, &err_stack),
      &err_stack);

  zx_u8 *f_compressed2 = nullptr;
  ZX_ERR_TRY_FATAL(
      alloc.alloc<zx_u8>(COMPRESSED_CAP, &f_compressed2, &err_stack),
      &err_stack);

  __builtin_memset(f_uncompressed, 0, UNCOMPRESSED_CAP);
  __builtin_memset(f_compressed, 0, COMPRESSED_CAP);
  __builtin_memset(f_compressed2, 0, COMPRESSED_CAP);

  FuzzCtx const ctx = {
      .compressed = f_compressed,
      .compressed2 = f_compressed2,
      .uncompressed = f_uncompressed,
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

  return 0;
}
