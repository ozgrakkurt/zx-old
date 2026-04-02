#ifndef ZX_PTR_CAST
#define ZX_PTR_CAST

#include <zx/zx_assert.h>
#include <zx/zx_def.h>

static_assert(sizeof(void *) == sizeof(zx_u64));
static_assert(alignof(void *) == alignof(zx_u64));

template <typename T> T *zx_ptr_cast_mut(void *p) {
  ZX_ASSERT(((zx_u64)p % alignof(T) == 0));
  ZX_ASSERT((p != nullptr));
  return (T *)p;
}

template <typename T> T const *zx_ptr_cast_const(void const *p) {
  ZX_ASSERT(((zx_u64)p % alignof(T) == 0));
  ZX_ASSERT((p != nullptr));
  return (T const *)p;
}

#endif
