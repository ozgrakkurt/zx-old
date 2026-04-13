#ifndef ZX_MEMMEM_H
#define ZX_MEMMEM_H

#include <zx/zx_def.h>
#include <zx/zx_slice.h>
#include <zx/zx_int.h>

static_assert(sizeof(void *) == sizeof(zx_u64));

// Tries to find the `needle` in the `haystack`. Returns true and a 
// `position` into the the `haystack` if found. Returns false if not found.
static inline bool zx_memmem(ZXSlice<zx_u8> haystack, ZXSlice<zx_u8> needle, zx_u64* position) const {
  if (needle.len() == 0) {
    *position = 0;
    return true;
  } else if (haystack.len() < needle.len()) {
    *position = ZX_U64_MAX;
    return false;
  } else if (haystack.len() == needle.len()) {
    if (haystack.is_eq(needle)) {
      *position = 0;
      return true;
    } else {
      *position = ZX_U64_MAX;
      return false;
    }
  }

  for (zx_u64 i = 0; i < haystack.len() - needle.len(); i++) {
    if (haystack.slice(i, needle.len()).is_eq(needle)) {
      *position = i;
      return true;
    }
  }

  *position = ZX_U64_MAX;
  return false;
}

#endif
