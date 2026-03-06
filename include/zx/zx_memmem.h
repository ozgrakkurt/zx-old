#ifndef ZX_MEMMEM_H
#define ZX_MEMMEM_H

#include <zx/zx_def.h>

static inline zx_u8 const *zx_memmem(zx_u8 const *haystack,
                                     zx_usize haystack_size,
                                     zx_u8 const *needle,
                                     zx_usize needle_size) {
  if (needle_size == 0) {
    return haystack;
  } else if (haystack_size < needle_size) {
    return nullptr;
  } else if (haystack_size == needle_size) {
    if (__builtin_memcmp(haystack, needle, haystack_size) == 0) {
      return haystack;
    } else {
      return nullptr;
    }
  }

  for (zx_usize i = 0; i < haystack_size - needle_size; i++) {
    if (__builtin_memcmp(&haystack[i], needle, needle_size) == 0) {
      return &haystack[i];
    }
  }

  return nullptr;
}

#endif
