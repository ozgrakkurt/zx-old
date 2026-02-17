#include <inttypes.h>
#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <shin/shin_page_alloc.h>
#include <shin/shin_time.h>
#include <shin/shin_xoshiro256.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <streamvbyte/streamvbyte.h>
#include <string.h>

typedef shin_u256 datatype;

constexpr shin_u32 LEN = 10123123;
constexpr shin_u32 MEM_ALIGN = 1 << 21;
constexpr shin_u32 MEMORY_SIZE =
    (LEN * sizeof(datatype) * 4 + MEM_ALIGN - 1) / MEM_ALIGN * MEM_ALIGN;

ShinStatus do_memcpy(shin_u32 const *restrict input,
                     shin_u8 *restrict compressed, shin_u32 *restrict output,
                     shin_u32 len, ShinErrStack *err_stack) {
  SHIN_ASSERT((input != nullptr));
  SHIN_ASSERT((compressed != nullptr));
  SHIN_ASSERT((output != nullptr));

  ShinTimer timer = ShinTimer_init();

  memcpy(compressed, input, len * sizeof(shin_u32));

  fprintf(stderr, "MemCpyIn in %" PRId64 " ticks\n",
          (uint64_t){ShinTimer_lap(&timer)});

  memcpy(output, compressed, len * sizeof(shin_u32));

  fprintf(stderr, "MemCpyOut in %" PRId64 " ticks\n",
          (uint64_t){ShinTimer_lap(&timer)});

  SHIN_ERR_ASSERT((memcmp(input, output, sizeof(shin_u32) * len) == 0),
                  err_stack);

  return ShinStatus_Ok;
}

ShinStatus do_bench(shin_u32 const *restrict input,
                    shin_u8 *restrict compressed, shin_u32 *restrict output,
                    shin_u32 len, ShinErrStack *err_stack) {
  SHIN_ASSERT((input != nullptr));
  SHIN_ASSERT((compressed != nullptr));
  SHIN_ASSERT((output != nullptr));

  shin_u32 compress_bound = 0;
  SHIN_ERR_TRY((streamvbyte_compress_bound(len, &compress_bound, err_stack)),
               err_stack);

  ShinTimer timer = ShinTimer_init();

  shin_u32 compressed_size = 0;
  SHIN_ERR_TRY((streamvbyte_0124_encode(input, len, compressed, compress_bound,
                                        &compressed_size, err_stack)),
               err_stack);

  fprintf(stderr, "Encode in %" PRId64 " ticks\n",
          (uint64_t){ShinTimer_lap(&timer)});

  SHIN_ERR_ASSERT((compressed_size <= compress_bound), err_stack);

  shin_u32 n_read = 0;
  SHIN_ERR_TRY((streamvbyte_0124_decode(compressed, compressed_size, output,
                                        len, &n_read, err_stack)),
               err_stack);

  fprintf(stderr, "Decode in %" PRId64 " ticks\n",
          (uint64_t){ShinTimer_lap(&timer)});

  SHIN_ERR_ASSERT((n_read <= compressed_size), err_stack);

  SHIN_ERR_ASSERT((memcmp(input, output, sizeof(shin_u32) * len) == 0),
                  err_stack);

  fprintf(
      stderr, "Ratio %f\n",
      (double){(shin_f64)(len * sizeof(shin_u32)) / (shin_f64)compressed_size});

  return ShinStatus_Ok;
}

ShinStatus roundtrip(ShinAlloc *alloc, ShinErrStack *err_stack) {
  constexpr shin_u32 RATIO = sizeof(datatype) / sizeof(shin_u32);

  datatype *input = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(alloc, sizeof(datatype) * LEN, (void **)&input,
                                err_stack)),
               err_stack);

  ShinXoshiro256 rand = ShinXoshiro256_init(0);
  for (shin_u32 i = 0; i < LEN; i++) {
    input[i] = (shin_u32)ShinXoshiro256_next(&rand);
  }

  shin_u32 compress_bound = 0;
  SHIN_ERR_TRY(
      (streamvbyte_compress_bound(LEN * RATIO, &compress_bound, err_stack)),
      err_stack);
  shin_u8 *compressed = nullptr;
  SHIN_ERR_TRY(
      (ShinAlloc_alloc(alloc, compress_bound, (void **)&compressed, err_stack)),
      err_stack);

  datatype *output = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(alloc, sizeof(datatype) * LEN, (void **)&output,
                                err_stack)),
               err_stack);

  memset(compressed, 0, compress_bound);
  memset(output, 0, sizeof(datatype) * LEN);

  SHIN_ERR_TRY((do_memcpy((shin_u32 const *)input, compressed,
                          (shin_u32 *)output, LEN * RATIO, err_stack)),
               err_stack);
  SHIN_ERR_TRY((do_bench((shin_u32 const *)input, compressed,
                         (shin_u32 *)output, LEN * RATIO, err_stack)),
               err_stack);

  return ShinStatus_Ok;
}

int main(void) {
  void *const page = shin_page_alloc_thp_2mb(MEMORY_SIZE);

  ShinErrStack err_stack = ShinErrStack_init();
  ShinAlloc alloc = ShinAlloc_init(page, MEMORY_SIZE);

  SHIN_ERR_TRACE_ABORT((roundtrip(&alloc, &err_stack)), &err_stack);

  shin_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}
