#include <noz/noz.h>
#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fmt.h>
#include <zx/zx_io.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_time.h>
#include <zx/zx_xoshiro256.h>

typedef zx_u256 datatype;

constexpr zx_u64 LEN = ((zx_u64){1} << 24) + 5;
constexpr zx_u64 MEMORY_SIZE = (zx_u64){1} << 31;

static ZXStatus do_memcpy(ZXFmt *fmt, zx_u32 const *__restrict input,
                          zx_u8 *__restrict compressed,
                          zx_u32 *__restrict output, zx_u32 len,
                          ZXErrStack *err_stack) {
  ZX_ASSERT((input != nullptr));
  ZX_ASSERT((compressed != nullptr));
  ZX_ASSERT((output != nullptr));

  ZXTimer timer = ZXTimer::init();

  __builtin_memcpy(compressed, input, len * sizeof(zx_u32));

  ZX_FMT(fmt, "MemCpyIn ", timer.lap(), " ticks\n");

  __builtin_memcpy(output, compressed, len * sizeof(zx_u32));

  ZX_FMT(fmt, "MemCpyOut ", timer.lap(), " ticks\n");

  ZX_ERR_ASSERT((__builtin_memcmp(input, output, sizeof(zx_u32) * len) == 0),
                err_stack);

  return ZXStatus::Ok;
}

ZXStatus do_bench(ZXFmt *fmt, zx_u32 const *__restrict input,
                  zx_u8 *__restrict compressed, zx_u32 *__restrict output,
                  zx_u32 len, ZXErrStack *err_stack) {
  ZX_ASSERT((input != nullptr));
  ZX_ASSERT((compressed != nullptr));
  ZX_ASSERT((output != nullptr));

  zx_u64 const size = (zx_u64){len} * sizeof(zx_u32);

  zx_u64 const compress_bound = noz_compress_bound(size);

  ZXTimer timer = ZXTimer::init();

  zx_u64 const compressed_size =
      noz_compress((zx_u8 const *)input, size, compressed, compress_bound);

  ZX_FMT(fmt, "Encode in ", timer.lap(), " ticks\n");

  ZX_ERR_ASSERT((compressed_size <= compress_bound), err_stack);

  zx_u64 const n_read = noz_decompress(compressed, (zx_u8 *)output, size);

  ZX_FMT(fmt, "Decode in ", timer.lap(), " ticks\n");

  ZX_ERR_ASSERT((n_read == compressed_size), err_stack);

  ZX_ERR_ASSERT((__builtin_memcmp(input, output, sizeof(zx_u32) * len) == 0),
                err_stack);

  zx_u64 const ratio =
      (zx_u64){compressed_size} * 100 / ((zx_u64){len} * sizeof(zx_u32));
  ZX_FMT(fmt, "Ratio ", ratio, "%\n");

  return ZXStatus::Ok;
}

ZXStatus do_bench_safe(ZXFmt *fmt, zx_u32 const *__restrict input,
                       zx_u8 *__restrict compressed, zx_u32 *__restrict output,
                       zx_u32 len, ZXErrStack *err_stack) {
  ZX_ASSERT((input != nullptr));
  ZX_ASSERT((compressed != nullptr));
  ZX_ASSERT((output != nullptr));

  zx_u64 const size = (zx_u64){len} * sizeof(zx_u32);

  zx_u64 const compress_bound = noz_compress_bound(size);

  ZXTimer timer = ZXTimer::init();

  zx_u64 const compressed_size =
      noz_compress((zx_u8 const *)input, size, compressed, compress_bound);

  ZX_FMT(fmt, "Encode for Safe in ", timer.lap(), " ticks\n");

  ZX_ERR_ASSERT((compressed_size <= compress_bound), err_stack);

  zx_u64 n_read = 0;
  ZX_ERR_TRY((noz_decompress_safe(compressed, compressed_size, (zx_u8 *)output,
                                  size, &n_read, err_stack)),
             err_stack);

  ZX_FMT(fmt, "DecodeSafe in ", timer.lap(), " ticks\n");

  ZX_ERR_ASSERT((n_read == compressed_size), err_stack);

  ZX_ERR_ASSERT((__builtin_memcmp(input, output, sizeof(zx_u32) * len) == 0),
                err_stack);

  zx_u64 const ratio =
      (zx_u64){compressed_size} * 100 / ((zx_u64){len} * sizeof(zx_u32));
  ZX_FMT(fmt, "Ratio ", ratio, "%\n");

  return ZXStatus::Ok;
}

static ZXStatus roundtrip(ZXAlloc *alloc, ZXErrStack *err_stack) {
  constexpr zx_u32 RATIO = sizeof(datatype) / sizeof(zx_u32);

  zx_u8 *nouse = nullptr;
  ZX_ERR_TRY((alloc->alloc<zx_u8>(69, &nouse, err_stack)), err_stack);

  datatype *input = nullptr;
  ZX_ERR_TRY((alloc->alloc<datatype>(LEN, &input, err_stack)), err_stack);

  ZXXoshiro256 rand = ZXXoshiro256::init(0);
  for (zx_u32 i = 0; i < LEN; i++) {
    input[i] = (zx_u32)rand.next();
  }

  nouse = nullptr;
  ZX_ERR_TRY((alloc->alloc<zx_u8>(69, &nouse, err_stack)), err_stack);

  zx_u64 const compress_bound =
      noz_compress_bound(LEN * RATIO * sizeof(zx_u32));
  zx_u8 *compressed = nullptr;
  ZX_ERR_TRY((alloc->alloc<zx_u8>(compress_bound, &compressed, err_stack)),
             err_stack);

  nouse = nullptr;
  ZX_ERR_TRY((alloc->alloc<zx_u8>(69, &nouse, err_stack)), err_stack);

  datatype *output = nullptr;
  ZX_ERR_TRY((alloc->alloc<datatype>(LEN, &output, err_stack)), err_stack);

  constexpr zx_u32 FMT_CAP = 1 << 12;
  char fmt_buf[FMT_CAP] = {0};
  ZXFmt fmt = ZXFmt::init(fmt_buf, FMT_CAP);

  rand.fill((zx_u8 *)compressed, compress_bound);
  rand.fill((zx_u8 *)output, sizeof(datatype) * LEN);

  ZX_ERR_TRY((do_memcpy(&fmt, (zx_u32 const *)input, compressed,
                        (zx_u32 *)output, LEN * RATIO, err_stack)),
             err_stack);

  rand.fill((zx_u8 *)compressed, compress_bound);
  rand.fill((zx_u8 *)output, sizeof(datatype) * LEN);

  ZX_ERR_TRY((do_bench(&fmt, (zx_u32 const *)input, compressed,
                       (zx_u32 *)output, LEN * RATIO, err_stack)),
             err_stack);

  rand.fill((zx_u8 *)compressed, compress_bound);
  rand.fill((zx_u8 *)output, sizeof(datatype) * LEN);

  ZX_ERR_TRY((do_bench_safe(&fmt, (zx_u32 const *)input, compressed,
                            (zx_u32 *)output, LEN * RATIO, err_stack)),
             err_stack);

  ZX_ERR_TRY_FATAL((zx_io_print(ZX_STDERR, &fmt, err_stack)), err_stack);

  return ZXStatus::Ok;
}

int main(void) {
  ZXErrStack err_stack = ZXErrStack::init();

  zx_u8 *mem = nullptr;
  ZX_ERR_TRY_FATAL((zx_page_alloc_huge_1gb(MEMORY_SIZE, &mem, &err_stack)),
                   &err_stack);

  ZXAlloc alloc = ZXAlloc::init(mem, MEMORY_SIZE);

  ZX_ERR_TRY_FATAL((roundtrip(&alloc, &err_stack)), &err_stack);

  ZX_ERR_TRY_FATAL((zx_page_alloc_free(mem, MEMORY_SIZE, &err_stack)),
                   &err_stack);
  return 0;
}
