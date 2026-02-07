#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <shin/shin_hashtable.h>
#include <shin/shin_rapidhash_v3.h>
#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerInitialize(int *argc, char ***argv) {
  (void)argc;
  (void)argv;

  return 0;
}

int LLVMFuzzerTestOneInput(uint8_t const *fuzz_input, size_t fuzz_input_len) {
  (void)fuzz_input;
  (void)fuzz_input_len;

  return 0;
}
