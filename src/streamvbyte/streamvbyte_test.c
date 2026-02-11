#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <shin/shin_page_alloc.h>
#include <shin/shin_xoshiro256.h>
#include <stdlib.h>
#include <streamvbyte/streamvbyte.h>
#include <string.h>

constexpr shin_usize MEMORY_SIZE = 1 << 20;

constexpr shin_usize MAX_N = 2048;

ShinStatus do_test(shin_u32 const *restrict input, shin_u8 *restrict compressed,
                   shin_u32 *restrict output, shin_u32 len,
                   ShinErrStack *err_stack) {
  SHIN_ASSERT((input != nullptr));
  SHIN_ASSERT((compressed != nullptr));
  SHIN_ASSERT((output != nullptr));

  shin_u32 compress_bound = 0;
  SHIN_ERR_TRY((streamvbyte_compress_bound(len, &compress_bound, err_stack)),
               err_stack);

  shin_u32 compressed_size = 0;
  SHIN_ERR_TRY((streamvbyte_0124_encode(input, len, compressed, compress_bound,
                                        &compressed_size, err_stack)),
               err_stack);

  SHIN_ERR_ASSERT((compressed_size <= compress_bound), err_stack);

  shin_u32 n_read = 0;
  SHIN_ERR_TRY((streamvbyte_0124_decode(compressed, compress_bound, output, len,
                                        &n_read, err_stack)),
               err_stack);

  SHIN_ERR_ASSERT((n_read <= compressed_size), err_stack);

  SHIN_ERR_ASSERT((memcmp(input, output, sizeof(shin_u32) * len) == 0),
                  err_stack);

  return ShinStatus_Ok;
}

ShinStatus roundtrip(ShinAlloc *alloc, ShinErrStack *err_stack) {
  shin_u32 *input = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(alloc, sizeof(shin_u32) * MAX_N,
                                (void **)&input, err_stack)),
               err_stack);
  ShinXoshiro256 rand = ShinXoshiro256_init(0);
  ShinXoshiro256_fill(&rand, (shin_u8 *)input, sizeof(shin_u32) * MAX_N);

  shin_u32 compress_bound = 0;
  SHIN_ERR_TRY((streamvbyte_compress_bound(MAX_N, &compress_bound, err_stack)),
               err_stack);
  shin_u8 *compressed = nullptr;
  SHIN_ERR_TRY(
      (ShinAlloc_alloc(alloc, compress_bound, (void **)&compressed, err_stack)),
      err_stack);

  shin_u32 *output = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(alloc, sizeof(shin_u32) * MAX_N,
                                (void **)&output, err_stack)),
               err_stack);

  constexpr shin_u32 LENGTHS[] = {0, 1, 69, 1033, 1024, 2009, 2048};
  constexpr shin_usize N_LENGTHS = sizeof(LENGTHS) / sizeof(LENGTHS[0]);

  for (shin_usize i = 0; i < N_LENGTHS; i++) {
    memset(compressed, 0, compress_bound);
    memset(output, 0, sizeof(shin_u32) * MAX_N);

    SHIN_ASSERT((MAX_N >= LENGTHS[i]));
    SHIN_ERR_TRY((do_test(input, compressed, output, LENGTHS[i], err_stack)),
                 err_stack);
  }

  return ShinStatus_Ok;
}

int main(void) {
  void *const page = shin_page_alloc_4kb(MEMORY_SIZE);

  ShinErrStack err_stack = ShinErrStack_init();
  ShinAlloc alloc = ShinAlloc_init(page, MEMORY_SIZE);

  SHIN_ERR_TRACE_ABORT((roundtrip(&alloc, &err_stack)), &err_stack);

  shin_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}
