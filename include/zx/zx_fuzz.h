#ifndef ZX_FUZZ_H
#define ZX_FUZZ_H

#ifdef ZX_FUZZING_ENABLED

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>

struct ZXFuzzInput {
  zx_u64 *input;
  zx_u32 input_len;
  zx_u32 consumed_count_;

  static inline ZXFuzzInput init(zx_u64 *input, zx_u32 input_len) {
    return ZXFuzzInput{
        .input = input,
        .input_len = input_len,
        .consumed_count_ = 0,
    };
  }

  __attribute__((always_inline)) inline zx_u64 next() {
    zx_u32 const offset = this->consumed_count_;
    ZX_ASSERT((this->input_len > offset));
    this->consumed_count_ = offset + 1;

    return this->input[offset];
  }

  __attribute__((always_inline)) inline zx_u32 consumed_count() const {
    return this->consumed_count_;
  }
};

typedef ZXStatus (*zx_fuzz_one_fn)(void *usr_ctx, ZXFuzzInput *fuzz_input,
                                   ZXErrStack *err_stack);

struct ZXFuzzParams {
  void *usr_ctx;
  zx_fuzz_one_fn fuzz_one;
  char const *tgt_file_filter;
  zx_u16 tgt_file_filter_len;
};

ZXStatus zx_fuzz_main(ZXAlloc *alloc, ZXFuzzParams const *params,
                      ZXErrStack *err_stack);

#endif // ifdef ZX_FUZZING_ENABLED

#endif
