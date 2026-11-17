#include <streamvbyte/streamvbyte.h>
#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_xoshiro256.h>

constexpr zx_u64 MEMORY_SIZE = 1 << 20;

constexpr zx_u64 MAX_N = 2048;

static ZXStatus do_test(zx_u32 const *restrict input,
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
  ZX_ERR_TRY((streamvbyte_0124_decode(compressed, compress_bound, output, len,
                                      &n_read, err_stack)),
             err_stack);

  ZX_ERR_ASSERT((n_read <= compressed_size), err_stack);

  ZX_ERR_ASSERT((__builtin_memcmp(input, output, sizeof(zx_u32) * len) == 0),
                err_stack);

  return ZXStatus_Ok;
}

static ZXStatus roundtrip(ZXAlloc *alloc, ZXErrStack *err_stack) {
  zx_u32 *input = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(zx_u32), sizeof(zx_u32) * MAX_N,
                            (zx_u8 **)&input, err_stack)),
             err_stack);
  ZXXoshiro256 rand = ZXXoshiro256_init(0);
  ZXXoshiro256_fill(&rand, (zx_u8 *)input, sizeof(zx_u32) * MAX_N);

  zx_u32 compress_bound = 0;
  ZX_ERR_TRY((streamvbyte_compress_bound(MAX_N, &compress_bound, err_stack)),
             err_stack);
  zx_u8 *compressed = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(zx_u8), compress_bound, &compressed,
                            err_stack)),
             err_stack);

  zx_u32 *output = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(zx_u32), sizeof(zx_u32) * MAX_N,
                            (zx_u8 **)&output, err_stack)),
             err_stack);

  constexpr zx_u32 LENGTHS[] = {0, 1, 69, 1033, 1024, 2009, 2048};
  constexpr zx_u64 N_LENGTHS = sizeof(LENGTHS) / sizeof(LENGTHS[0]);

  for (zx_u64 i = 0; i < N_LENGTHS; i++) {
    __builtin_memset(compressed, 0, compress_bound);
    __builtin_memset(output, 0, sizeof(zx_u32) * MAX_N);

    ZX_ASSERT((MAX_N >= LENGTHS[i]));
    ZX_ERR_TRY((do_test(input, compressed, output, LENGTHS[i], err_stack)),
               err_stack);
  }

  return ZXStatus_Ok;
}

int main(void) {
  ZXErrStack err_stack = ZXErrStack_init();

  zx_u8 *mem = nullptr;
  ZX_ERR_TRY_FATAL((zx_page_alloc_4kb(MEMORY_SIZE, &mem, &err_stack)),
                   &err_stack);

  ZXAlloc alloc = ZXAlloc_init(mem, MEMORY_SIZE);

  ZX_ERR_TRY_FATAL((roundtrip(&alloc, &err_stack)), &err_stack);

  ZX_ERR_TRY_FATAL((zx_page_alloc_free(mem, MEMORY_SIZE, &err_stack)),
                   &err_stack);
  return 0;
}
