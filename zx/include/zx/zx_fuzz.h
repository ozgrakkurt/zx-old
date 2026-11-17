#ifndef ZX_FUZZ_H
#define ZX_FUZZ_H

#ifdef ZX_FUZZING_ENABLED

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>

typedef struct ZXFuzzInput {
  zx_u64 *input;
  zx_u32 input_len;
  zx_u32 consumed_count;
} ZXFuzzInput;

static inline ZXFuzzInput ZXFuzzInput_init(zx_u64 *input, zx_u32 input_len) {
  return (ZXFuzzInput){
      .input = input,
      .input_len = input_len,
      .consumed_count = 0,
  };
}

static inline zx_u64 ZXFuzzInput_next(ZXFuzzInput *input) {
  zx_u32 const offset = input->consumed_count;
  ZX_ASSERT((input->input_len > offset));
  input->consumed_count = offset + 1;

  return input->input[offset];
}

static inline zx_u32 ZXFuzzInput_consumed_count(ZXFuzzInput const *input) {
  return input->consumed_count;
}

typedef ZXStatus (*zx_fuzz_one_fn)(void *usr_ctx, ZXFuzzInput *fuzz_input,
                                   ZXErrStack *err_stack);

typedef struct ZXFuzzParams {
  void *usr_ctx;
  zx_fuzz_one_fn fuzz_one;
  char const *tgt_file_filter;
  zx_u16 tgt_file_filter_len;
} ZXFuzzParams;

ZXStatus zx_fuzz_main(ZXAlloc *alloc, ZXFuzzParams const *params,
                      ZXErrStack *err_stack);

#endif // ifdef ZX_FUZZING_ENABLED

#endif
