#include <streamvbyte/streamvbyte.h>
#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fmt.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>
#include <zx/zx_time.h>
#include <zx/zx_xoshiro256.h>

typedef zx_u256 datatype;

constexpr zx_u32 LEN = 10123123;
constexpr zx_u32 MEM_ALIGN = 1 << 21;
constexpr zx_u32 MEMORY_SIZE =
    (LEN * sizeof(datatype) * 4 + MEM_ALIGN - 1) / MEM_ALIGN * MEM_ALIGN;

static ZXStatus do_memcpy(ZXFmt *fmt, zx_u32 const *restrict input,
                          zx_u8 *restrict compressed, zx_u32 *restrict output,
                          zx_u32 len, ZXErrStack *err_stack) {
  ZX_ASSERT((input != nullptr));
  ZX_ASSERT((compressed != nullptr));
  ZX_ASSERT((output != nullptr));

  ZXTimer timer = ZXTimer_init();

  __builtin_memcpy(compressed, input, len * sizeof(zx_u32));

  ZX_FMT(fmt, "MemCpyIn ", ZXTimer_lap(&timer), " ticks\n");

  __builtin_memcpy(output, compressed, len * sizeof(zx_u32));

  ZX_FMT(fmt, "MemCpyOut ", ZXTimer_lap(&timer), " ticks\n");

  ZX_ERR_ASSERT((__builtin_memcmp(input, output, sizeof(zx_u32) * len) == 0),
                err_stack);

  return ZXStatus_Ok;
}

ZXStatus do_bench(ZXFmt *fmt, zx_u32 const *restrict input,
                  zx_u8 *restrict compressed, zx_u32 *restrict output,
                  zx_u32 len, ZXErrStack *err_stack) {
  ZX_ASSERT((input != nullptr));
  ZX_ASSERT((compressed != nullptr));
  ZX_ASSERT((output != nullptr));

  zx_u32 compress_bound = 0;
  ZX_ERR_TRY((streamvbyte_compress_bound(len, &compress_bound, err_stack)),
             err_stack);

  ZXTimer timer = ZXTimer_init();

  zx_u32 compressed_size = 0;
  ZX_ERR_TRY((streamvbyte_0124_encode(input, len, compressed, compress_bound,
                                      &compressed_size, err_stack)),
             err_stack);

  ZX_FMT(fmt, "Encode in ", ZXTimer_lap(&timer), " ticks\n");

  ZX_ERR_ASSERT((compressed_size <= compress_bound), err_stack);

  zx_u32 n_read = 0;
  ZX_ERR_TRY((streamvbyte_0124_decode(compressed, compressed_size, output, len,
                                      &n_read, err_stack)),
             err_stack);

  ZX_FMT(fmt, "Decode in ", ZXTimer_lap(&timer), " ticks\n");

  ZX_ERR_ASSERT((n_read <= compressed_size), err_stack);

  ZX_ERR_ASSERT((__builtin_memcmp(input, output, sizeof(zx_u32) * len) == 0),
                err_stack);

  zx_u64 const ratio =
      (zx_u64){compressed_size} * 100wbu / ((zx_u64){len} * sizeof(zx_u32));
  ZX_FMT(fmt, "Ratio ", ratio, "%\n");

  return ZXStatus_Ok;
}

static ZXStatus roundtrip(ZXAlloc *alloc, ZXErrStack *err_stack) {
  constexpr zx_u32 RATIO = sizeof(datatype) / sizeof(zx_u32);

  datatype *input = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(datatype), sizeof(datatype) * LEN,
                            (zx_u8 **)&input, err_stack)),
             err_stack);

  ZXXoshiro256 rand = ZXXoshiro256_init(0);
  for (zx_u32 i = 0; i < LEN; i++) {
    input[i] = (zx_u32)ZXXoshiro256_next(&rand);
  }

  zx_u32 compress_bound = 0;
  ZX_ERR_TRY(
      (streamvbyte_compress_bound(LEN * RATIO, &compress_bound, err_stack)),
      err_stack);
  zx_u8 *compressed = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(zx_u8), compress_bound, &compressed,
                            err_stack)),
             err_stack);

  datatype *output = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(datatype), sizeof(datatype) * LEN,
                            (zx_u8 **)&output, err_stack)),
             err_stack);

  constexpr zx_u32 FMT_CAP = 1 << 12;
  char fmt_buf[FMT_CAP] = {0};
  ZXFmt fmt = ZXFmt_init(fmt_buf, FMT_CAP);

  __builtin_memset(compressed, 0, compress_bound);
  __builtin_memset(output, 0, sizeof(datatype) * LEN);

  ZX_ERR_TRY((do_bench(&fmt, (zx_u32 const *)input, compressed,
                       (zx_u32 *)output, LEN * RATIO, err_stack)),
             err_stack);

  __builtin_memset(compressed, 0, compress_bound);
  __builtin_memset(output, 0, sizeof(datatype) * LEN);

  ZX_ERR_TRY((do_memcpy(&fmt, (zx_u32 const *)input, compressed,
                        (zx_u32 *)output, LEN * RATIO, err_stack)),
             err_stack);

  zx_u64 n_written = 0;
  ZX_ERR_TRY((zx_sys_write(ZX_STDERR, (zx_u8 const *)fmt_buf,
                           ZXFmt_n_written(&fmt), &n_written, err_stack)),
             err_stack);
  (void)n_written;

  return ZXStatus_Ok;
}

int main(void) {
  ZXErrStack err_stack = ZXErrStack_init();

  zx_u8 *mem = nullptr;
  ZX_ERR_TRY_FATAL((zx_page_alloc_thp_2mb(MEMORY_SIZE, &mem, &err_stack)),
                   &err_stack);

  ZXAlloc alloc = ZXAlloc_init(mem, MEMORY_SIZE);

  ZX_ERR_TRY_FATAL((roundtrip(&alloc, &err_stack)), &err_stack);

  ZX_ERR_TRY_FATAL((zx_page_alloc_free(mem, MEMORY_SIZE, &err_stack)),
                   &err_stack);
  return 0;
}
