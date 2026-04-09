#ifndef ZX_FUZZ_H
#define ZX_FUZZ_H

#ifdef ZX_FUZZING_ENABLED

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>

struct ZXFuzzInput {
  ZXSlice<zx_u64> m_input;
  zx_u64 m_consumed_count;

  static inline ZXFuzzInput init(ZXSlice<zx_u64> input) {
    return ZXFuzzInput{
        .m_input = input,
        .m_consumed_count = 0,
    };
  }

  __attribute__((always_inline)) inline zx_u64 next() {
    zx_u32 const offset = this->m_consumed_count;
    ZX_ASSERT((this->m_input.len() > offset));
    this->m_consumed_count = offset + 1;
    return this->m_input.ptr()[offset];
  }

  __attribute__((always_inline)) inline zx_u64 consumed_count() const {
    return this->m_consumed_count;
  }
};

typedef ZXStatus (*zx_fuzz_one_fn)(void *usr_ctx, ZXFuzzInput *fuzz_input,
                                   ZXErrStack *err_stack);

struct ZXFuzzParams {
  void *usr_ctx;
  zx_fuzz_one_fn fuzz_one;
  ZXSlice<char> tgt_file_filter;
};

ZXStatus zx_fuzz_main(ZXAlloc alloc, ZXFuzzParams const *params,
                      ZXErrStack *err_stack);

#endif // ifdef ZX_FUZZING_ENABLED

#endif
