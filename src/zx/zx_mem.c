#include <zx/zx_def.h>

#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

typedef unsigned long sizet;
static_assert(sizeof(void *) == sizeof(sizet));

__attribute__((no_builtin)) void *
memcpy(void *restrict dest, const void *restrict src, sizet count) {
  void *const orig_dest = dest;
  __asm__("rep movsb\n\t"
          : "+D"(dest), "+S"(src), "+c"(count), "=m"(*(zx_u8(*)[count])dest)
          : "m"(*(zx_u8 const(*)[count])src));
  return orig_dest;
}

__attribute__((no_builtin)) void *memset(void *const dest, int ch,
                                         sizet count) {
  void *ptr = dest;
  unsigned char val = (unsigned char)ch;

  __asm__("rep stosb\n\t"
          : "+D"(ptr), "+c"(count), "=m"(*(zx_u8(*)[count])dest)
          : "a"(val));

  return dest;
}

__attribute__((no_builtin)) void *memmove(void *dest, const void *src,
                                          sizet count) {
  if (count == 0 || dest == src) {
    return dest;
  }

  void *const orig_dest = dest;

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

  return orig_dest;
}

__attribute__((no_builtin)) sizet strlen(char const *src) {
  sizet count = ~((sizet)0);

  __asm__("repne scasb\n\t"
          : "+c"(count), "+D"(src)
          : "a"(0), "m"(*(zx_u8 const(*)[count])src));

  return ((sizet)0) - count - 2;
}

__attribute__((no_builtin)) int memcmp(void const *lhs, void const *rhs,
                                       sizet count) {
  if (count == 0)
    return 0;

  int result;
  char a;
  char b;
  __asm__("repe cmpsb\n\t"
          "seta %[a]\n\t"
          "setb %[b]\n\t"
          "subb %[b], %[a]\n\t"
          "movsbl %[a], %[result]\n\t"
          : [a] "=r"(a), [b] "=r"(b), "+S"(lhs), "+D"(rhs),
            "+c"(count), [result] "=r"(result)
          : "m"(*(zx_u8 const(*)[count])lhs), "m"(*(zx_u8 const(*)[count])rhs)
          : "cc");

  return result;
}
