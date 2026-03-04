#ifndef ZX_FUZZ_H
#define ZX_FUZZ_H

#ifdef ZX_FUZZING_ENABLED

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

static inline ZXStatus ZXFuzzInput_next(ZXFuzzInput *input, zx_u64 *out,
                                        ZXErrStack *err_stack) {
  zx_u32 const offset = input->consumed_count;

  if (input->input_len == offset) {
    ZX_ERR_RETURN((ZXStatus_OutOfInput), err_stack);
  }

  input->consumed_count = offset + 1;
  *out = input->input[offset];

  return ZXStatus_Ok;
}

static inline zx_u32 ZXFuzzInput_consumed_count(ZXFuzzInput const *input) {
  return input->consumed_count;
}

ZXStatus zx_fuzz_setup(void **usr_ctx, ZXErrStack *err_stack);
ZXStatus zx_fuzz_one(void *usr_ctx, ZXFuzzInput *fuzz_input,
                     ZXErrStack *err_stack);

#endif // ifdef ZX_FUZZING_ENABLED

#endif
