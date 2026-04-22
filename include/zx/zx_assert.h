#ifndef ZX_ASSERT_H
#define ZX_ASSERT_H

static inline void zx_assert_unsafe(bool test) {
#ifdef ZX_DEBUG
  if (!test) {
    __builtin_trap();
  }
#else
  (void)test;
#endif
}

#endif
