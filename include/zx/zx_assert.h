#ifndef ZX_ASSERT_H
#define ZX_ASSERT_H

static inline void zx_assert_dev(bool cond) {
#ifdef ZX_DEV
  if (!cond) {
    __builtin_trap();
  }
#else
  (void)cond;
#endif
}

#endif
