#ifndef ZX_MEMMEM_H
#define ZX_MEMMEM_H

#include <zx/zx_def.h>

static_assert(sizeof(void *) == sizeof(zx_u64));

static inline zx_u8 const *zx_memmem(void const *haystack, zx_u64 haystack_size,
                                     void const *needle, zx_u64 needle_size) {
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

  for (zx_u64 i = 0; i < haystack_size - needle_size; i++) {
    if (__builtin_memcmp(&haystack[i], needle, needle_size) == 0) {
      return &haystack[i];
    }
  }

  return nullptr;
}

#endif
