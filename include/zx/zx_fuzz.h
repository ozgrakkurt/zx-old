#ifndef ZX_FUZZ_H
#define ZX_FUZZ_H

#ifndef ZX_FUZZ
#error "ZX_FUZZ has to be defined to use this header."
#endif

#include <zx/zx_alloc.h>
#include <zx/zx_int.h>
#include <zx/zx_panic.h>
#include <zx/zx_slice.h>

struct ZXFuzzInput {
  ZXSlice<zx_u64> m_input;
  zx_u64 m_consumed_count;

  static inline ZXFuzzInput init_(ZXSlice<zx_u64> input) {
    return ZXFuzzInput{
        .m_input = input,
        .m_consumed_count = 0,
    };
  }

  inline zx_u64 next() {
    zx_u64 const offset = this->m_consumed_count;
    this->m_consumed_count = zx_add_unsafe_u64(offset, 1);
    if (this->m_input.len() <= offset) {
      zx_panic();
    }
    return this->m_input.read_unsafe(offset);
  }

  inline zx_u64 consumed_count_() const { return this->m_consumed_count; }
};

typedef void (*zx_fuzz_one_fn)(void *usr_ctx, ZXFuzzInput *fuzz_input);
void zx_fuzz(zx_u64 max_n_iter, ZXAlloc *alloc, void *usr_ctx, zx_fuzz_one_fn fuzz_one);

#endif
