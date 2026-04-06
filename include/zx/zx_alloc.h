#ifndef ZX_ALLOC_H
#define ZX_ALLOC_H

#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>

// Maximum alignment an allocation can be requested with. In bytes.
constexpr zx_u16 ZX_ALLOC_MAX_ALIGN = 128;

// This is the function that should be implemented by a concrete implementation
// of the `ZXAlloc` interface.
//
// Assigns the pointer to the allocated memory to `*ptr`.
//
// `align` can't be greater than `ZX_ALLOC_MAX_ALIGN`.
typedef ZXStatus (*zx_allocfn)(zx_u16 align, zx_u64 size, void **ptr,
                               ZXErrStack *err_stack);

struct ZXAlloc {
  void *ctx;
  zx_allocfn allocfunc;

  // returns ZXStatus::OutOfMemory if there is no more capacity to
  // allocate the memory in.
  template <typename T>
  inline ZXStatus aligned_alloc(zx_u16 align, zx_u64 len, T **ptr,
                                ZXErrStack *err_stack) const {
    ZX_ASSERT((err_stack != nullptr));
    ZX_ASSERT((ptr != nullptr));
    ZX_ASSERT((*ptr == nullptr));

    ZX_ASSERT((align <= ZX_ALLOC_MAX_ALIGN));
    ZX_ASSERT((align > 0));
    ZX_ASSERT((zx_is_pow_two_u16(align)));
    ZX_ASSERT((alignof(T) <= align));

    zx_u64 const size = zx_chk_mul_u64(len, sizeof(T));
    ZX_ERR_TRY((this->allocfunc(align, size, (void **)ptr, err_stack)),
               err_stack);

    T *vptr = *ptr;
    ZX_ASSERT((vptr != nullptr));
    static_assert(sizeof(void *) == sizeof(zx_u64));
    ZX_ASSERT(((zx_u64)vptr % align == 0));

    return ZXStatus::Ok;
  }

  template <typename T>
  __attribute__((always_inline)) inline ZXStatus
  alloc(zx_u64 len, T **ptr, ZXErrStack *err_stack) const {
    ZX_ERR_TRY((this->aligned_alloc<T>(alignof(T), len, ptr, err_stack)),
               err_stack);
    return ZXStatus::Ok;
  }
};

#endif
