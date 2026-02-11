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
    (sizeof(shin_u32) * MAX_N + MEM_ALIGN - 1) / MEM_ALIGN * MEM_ALIGN;

_Thread_local shin_u32 *OUTPUT;
_Thread_local shin_u32 COMPRESS_BOUND;

int LLVMFuzzerInitialize(int *argc, char ***argv) {
  (void)argc;
  (void)argv;

  void *mem = shin_page_alloc_4kb(MEM_SIZE);

  ShinAlloc alloc = ShinAlloc_init(mem, MEM_SIZE);
  ShinErrStack err_stack = ShinErrStack_init();

  shin_u32 *output = nullptr;
  SHIN_ERR_TRACE_ABORT((ShinAlloc_alloc(&alloc, sizeof(shin_u32) * MAX_N,
                                        (void **)&output, &err_stack)),
                       &err_stack);

  OUTPUT = output;

  return 0;
}

ShinStatus do_decode(shin_u8 const *restrict compressed,
                     shin_u32 compressed_size, shin_u32 *restrict output,
                     shin_u32 len, ShinErrStack *err_stack) {
  SHIN_ASSERT((compressed != nullptr));
  SHIN_ASSERT((output != nullptr));

  shin_u32 n_read = 0;
  ShinStatus const status = streamvbyte_0124_decode(
      compressed, compressed_size, output, len, &n_read, err_stack);
  // ignore error, we are only looking for crashes or invalid success
  if (status != ShinStatus_Ok) {
    return ShinStatus_Ok;
  }

  SHIN_ERR_ASSERT((n_read <= compressed_size), err_stack);

  return ShinStatus_Ok;
}

ShinStatus test_one(shin_u8 const *compressed, shin_u32 compressed_size,
                    shin_u32 len, ShinErrStack *err_stack) {
  memset(OUTPUT, 0, sizeof(shin_u32) * MAX_N);

  SHIN_ERR_TRY((do_decode(compressed, compressed_size, (shin_u32 *)OUTPUT, len,
                          err_stack)),
               err_stack);

  return ShinStatus_Ok;
}

int LLVMFuzzerTestOneInput(uint8_t const *fuzz_input, size_t fuzz_input_len) {
  constexpr shin_u32 HEAD_SIZE = sizeof(shin_u32);

  if (fuzz_input_len < HEAD_SIZE) {
    return 0;
  }

  shin_u32 len = 0;
  memcpy(&len, fuzz_input, sizeof(shin_u32));

  shin_u32 const compressed_size = ((shin_u32)fuzz_input_len) - HEAD_SIZE;

  ShinErrStack err_stack = ShinErrStack_init();

  SHIN_ERR_TRACE_ABORT((test_one((shin_u8 const *)&fuzz_input[HEAD_SIZE],
                                 compressed_size, len, &err_stack)),
                       &err_stack);

  return 0;
}
