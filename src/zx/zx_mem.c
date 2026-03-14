#include <zx/zx_def.h>

#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

typedef unsigned long sizet;
static_assert(sizeof(void *) == sizeof(sizet));

void *memcpy(void *restrict dest, const void *restrict src, sizet count) {
  void *orig_dest = dest;
  __asm__("rep movsb\n\t"
          : "+D"(dest), "+S"(src), "+c"(count), "=m"(*(zx_u8(*)[count])dest)
          : "m"(*(zx_u8 const(*)[count])src));
  return orig_dest;
}

void *memset(void *dest, int ch, sizet count) {
  void *ptr = dest;
  unsigned char val = (unsigned char)ch;

  __asm__("rep stosb\n\t"
          : "+D"(ptr), "+c"(count), "=m"(*(zx_u8(*)[count])dest)
          : "a"(val));

  return dest;
}

void *memmove(void *dest, const void *src, sizet count) {
  if (count == 0 || dest == src)
    return dest;

  if (dest < src || ((sizet)src + count <= (sizet)dest)) {
    // forwards
    __asm__("rep movsb\n\t"
            : "+D"(dest), "+S"(src), "+c"(count), "=m"(*(zx_u8(*)[count])dest)
            : "m"(*(zx_u8 const(*)[count])src));
  } else {
    // backwards
    void *d_end = (char *)dest + count - 1;
    const void *s_end = (const char *)src + count - 1;

    __asm__("std\n\t"
            "rep movsb\n\t"
            "cld\n\t"
            : "+D"(d_end), "+S"(s_end), "+c"(count),
              "=m"(*(zx_u8(*)[count])dest)
            : "m"(*(zx_u8 const(*)[count])src));
  }

  return dest;
}

__attribute__((no_builtin)) sizet strlen(char const *src) {
  zx_u8 const *sp = (zx_u8 const *)src;
  sizet count = 0;
  while (*sp++ != 0)
    count++;
  return count;
}

__attribute__((no_builtin)) int memcmp(void const *lhs, void const *rhs,
                                       sizet count) {
  zx_u8 const *lhsp = (zx_u8 const *)lhs;
  zx_u8 const *rhsp = (zx_u8 const *)rhs;

  for (sizet i = 0; i < count; i++) {
    if (lhsp[i] != rhsp[i]) {
      return (int)(lhsp[i] - rhsp[i]);
    }
  }

  return 0;
}
