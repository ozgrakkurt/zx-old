#include <noz/noz.h>
#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_xoshiro256.h>

constexpr zx_u64 MEMORY_SIZE = 1 << 20;
constexpr zx_u64 MAX_SIZE = 2048;

static ZXStatus do_test(zx_u8 const *__restrict input,
                        zx_u8 *__restrict compressed, zx_u8 *__restrict output,
                        zx_u64 size, ZXErrStack *err_stack) {
  ZX_ASSERT((input != nullptr));
  ZX_ASSERT((compressed != nullptr));
  ZX_ASSERT((output != nullptr));

  zx_u64 const compress_bound = noz_compress_bound(size);

  zx_u64 const compressed_size =
      noz_compress(input, size, compressed, compress_bound);
  ZX_ERR_ASSERT((compressed_size <= compress_bound), err_stack);

  // test unsafe variant
  __builtin_memset(output, 0, size);
  zx_u64 const n_read = noz_decompress(compressed, output, size);
  ZX_ERR_ASSERT((n_read == compressed_size), err_stack);
  ZX_ERR_ASSERT((__builtin_memcmp(input, output, size) == 0), err_stack);

  // test safe variant
  __builtin_memset(output, 0, size);
  zx_u64 n_read_safe = 0;
  ZX_ERR_TRY((noz_decompress_safe(compressed, compress_bound, output, size,
                                  &n_read_safe, err_stack)),
             err_stack);
  ZX_ERR_ASSERT((n_read_safe == compressed_size), err_stack);
  ZX_ERR_ASSERT((__builtin_memcmp(input, output, size) == 0), err_stack);

  // try again with passing compressed_size instead of compress_bound to
  // decompress function
  __builtin_memset(output, 0, size);
  n_read_safe = 0;
  ZX_ERR_TRY((noz_decompress_safe(compressed, compressed_size, output, size,
                                  &n_read_safe, err_stack)),
             err_stack);
  ZX_ERR_ASSERT((n_read_safe == compressed_size), err_stack);
  ZX_ERR_ASSERT((__builtin_memcmp(input, output, size) == 0), err_stack);

  return ZXStatus::Ok;
}

static ZXStatus roundtrip(ZXAlloc alloc, ZXErrStack *err_stack) {
  zx_u8 *input = nullptr;
  ZX_ERR_TRY((alloc.alloc<zx_u8>(MAX_SIZE, &input, err_stack)), err_stack);
  ZXXoshiro256 rand = ZXXoshiro256::init(0);
  rand.fill((zx_u8 *)input, MAX_SIZE);

  zx_u64 const compress_bound = noz_compress_bound(MAX_SIZE);
  zx_u8 *compressed = nullptr;
  ZX_ERR_TRY((alloc.alloc<zx_u8>(compress_bound, &compressed, err_stack)),
             err_stack);

  zx_u8 *output = nullptr;
  ZX_ERR_TRY((alloc.alloc<zx_u8>(MAX_SIZE, &output, err_stack)), err_stack);

  constexpr zx_u64 SIZES[] = {0, 1, 69, 1033, 1024, 2009, 2048};
  constexpr zx_u64 N_SIZES = sizeof(SIZES) / sizeof(SIZES[0]);

  // TODO: align buffers to right so we hit oob errors.
  for (zx_u64 i = 0; i < N_SIZES; i++) {
    __builtin_memset(compressed, 0, compress_bound);
    __builtin_memset(output, 0, MAX_SIZE);

    ZX_ASSERT((MAX_SIZE >= SIZES[i]));
    ZX_ERR_TRY((do_test(input, compressed, output, SIZES[i], err_stack)),
               err_stack);
  }

  return ZXStatus::Ok;
}

int main(void) {
  ZXErrStack err_stack = ZXErrStack::init();

  ZXDebugAlloc dbg_alloc;
  ZX_ERR_TRY_FATAL((ZXDebugAlloc::init(MEMORY_SIZE, &dbg_alloc, &err_stack)),
                   &err_stack);
  ZXAlloc alloc = dbg_alloc.make_alloc();

  ZX_ERR_TRY_FATAL((roundtrip(alloc, &err_stack)), &err_stack);
  return 0;
}
