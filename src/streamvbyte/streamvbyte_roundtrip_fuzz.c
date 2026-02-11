#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <shin/shin_page_alloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <streamvbyte/streamvbyte.h>
#include <string.h>

constexpr shin_usize MEM_ALIGN = 1 << 21;
constexpr shin_u32 MAX_N = 2048;
constexpr shin_usize MEM_SIZE =
    (sizeof(shin_u32) * MAX_N * 8 + MEM_ALIGN - 1) / MEM_ALIGN * MEM_ALIGN;

_Thread_local shin_u32 *INPUT;
_Thread_local shin_u8 *COMPRESSED;
_Thread_local shin_u32 *OUTPUT;
_Thread_local shin_u32 COMPRESS_BOUND;

int LLVMFuzzerInitialize(int *argc, char ***argv) {
  (void)argc;
  (void)argv;

  void *mem = shin_page_alloc_4kb(MEM_SIZE);

  ShinAlloc alloc = ShinAlloc_init(mem, MEM_SIZE);
  ShinErrStack err_stack = ShinErrStack_init();

  shin_u32 *input = nullptr;
  SHIN_ERR_TRACE_ABORT((ShinAlloc_alloc(&alloc, sizeof(shin_u32) * MAX_N,
                                        (void **)&input, &err_stack)),
                       &err_stack);

  shin_u32 compress_bound = 0;
  SHIN_ERR_TRACE_ABORT(
      (streamvbyte_compress_bound(MAX_N, &compress_bound, &err_stack)),
      &err_stack);
  shin_u8 *compressed = nullptr;
  SHIN_ERR_TRACE_ABORT((ShinAlloc_alloc(&alloc, compress_bound,
                                        (void **)&compressed, &err_stack)),
                       &err_stack);

  shin_u32 *output = nullptr;
  SHIN_ERR_TRACE_ABORT((ShinAlloc_alloc(&alloc, sizeof(shin_u32) * MAX_N,
                                        (void **)&output, &err_stack)),
                       &err_stack);

  INPUT = input;
  COMPRESSED = compressed;
  OUTPUT = output;
  COMPRESS_BOUND = compress_bound;

  return 0;
}

ShinStatus roundtrip(shin_u32 const *restrict input,
                     shin_u8 *restrict compressed, shin_u32 *restrict output,
                     shin_u32 len, ShinErrStack *err_stack) {
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

ShinStatus test_one(shin_u32 len, ShinErrStack *err_stack) {
  memset(COMPRESSED, 0, COMPRESS_BOUND);
  memset(OUTPUT, 0, sizeof(shin_u32) * MAX_N);

  SHIN_ERR_ASSERT((len <= MAX_N), err_stack);

  SHIN_ERR_TRY((roundtrip((shin_u32 const *)INPUT, (shin_u8 *)COMPRESSED,
                          (shin_u32 *)OUTPUT, len, err_stack)),
               err_stack);

  return ShinStatus_Ok;
}

int LLVMFuzzerTestOneInput(uint8_t const *fuzz_input, size_t fuzz_input_len) {
  constexpr shin_usize HEAD_SIZE = sizeof(shin_u32);

  if (fuzz_input_len <= HEAD_SIZE) {
    return 0;
  }

  shin_u32 len_in = 0;
  memcpy(&len_in, fuzz_input, sizeof(shin_u32));

  shin_u32 len = (len_in % (fuzz_input_len - HEAD_SIZE)) / sizeof(shin_u32);
  len %= MAX_N;

  memcpy(INPUT, fuzz_input, sizeof(shin_u32) * len);

  ShinErrStack err_stack = ShinErrStack_init();

  SHIN_ERR_TRACE_ABORT((test_one(len, &err_stack)), &err_stack);

  return 0;
}
