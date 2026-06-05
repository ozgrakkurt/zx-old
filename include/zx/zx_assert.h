#ifndef ZX_ASSERT_H
#define ZX_ASSERT_H

#include <zx/zx_panic.h>

static inline void zx_assert_always(bool cond) {
  if (!cond) [[unlikely]] {
    // zx_panic();
    __builtin_trap();
  }
}

static inline void zx_assert_dev(bool cond) {
#ifdef ZX_DEV
  if (!cond) [[unlikely]] {
    zx_panic();
  }
#else
  (void)cond;
#endif
}

#endif
