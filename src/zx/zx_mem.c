#include <zx/zx_def.h>

typedef unsigned long zx_size_type;
static_assert(sizeof(zx_size_type) == sizeof(void *));

__attribute__((no_builtin)) void *
memcpy(void *restrict dest, void const *restrict src, zx_size_type count) {
  zx_u8 *restrict dp = (zx_u8 *)dest;
  zx_u8 const *restrict sp = (zx_u8 const *)src;

  for (zx_size_type i = 0; i < count; i++) {
    dp[i] = sp[i];
  }

  return dest;
}

__attribute__((no_builtin)) void *memset(void *dest, int ch,
                                         zx_size_type count) {
  zx_u8 *dp = (zx_u8 *)dest;
  zx_u8 c = (zx_u8)ch;

  for (zx_size_type i = 0; i < count; i++) {
    dp[i] = c;
  }

  return dest;
}

__attribute__((no_builtin)) void *memmove(void *dest, void const *src,
                                          zx_size_type count) {
  zx_u8 *dp = (zx_u8 *)dest;
  zx_u8 const *sp = (zx_u8 const *)src;

  if (dp < sp) {
    for (zx_size_type i = 0; i < count; i++) {
      dp[i] = sp[i];
    }
  } else if (dp > sp) {
    for (zx_size_type i = count; i > 0; i--) {
      dp[i - 1] = sp[i - 1];
    }
  }

  return dest;
}

__attribute__((no_builtin)) zx_size_type strlen(char const *src) {
  zx_u8 const *sp = (zx_u8 const *)src;
  zx_size_type count = 0;
  while (*sp++ != 0)
    count++;
  return count;
}

__attribute__((no_builtin)) int memcmp(void const *lhs, void const *rhs,
                                       zx_size_type count) {
  zx_u8 const *lhsp = (zx_u8 const *)lhs;
  zx_u8 const *rhsp = (zx_u8 const *)rhs;

  for (zx_size_type i = 0; i < count; i++) {
    if (lhsp[i] != rhsp[i]) {
      return (int)(lhsp[i] - rhsp[i]);
    }
  }

  return 0;
}
