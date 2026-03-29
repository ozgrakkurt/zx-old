#ifndef ZX_ALLOC_H
#define ZX_ALLOC_H

#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>

struct ZXAlloc {
  zx_u64 const capacity;
  zx_u64 offset;
  zx_u8 *const buf;

  static inline ZXAlloc init(zx_u8 *buf, zx_u64 capacity) {
    ZX_ASSERT((buf != nullptr));

    return ZXAlloc{
        .capacity = capacity,
        .offset = 0,
        .buf = buf,
    };
  }

  inline void reset() { this->offset = 0; }

  template <typename T>
  inline ZXStatus aligned_alloc(zx_u16 align, zx_u64 len, T **ptr,
                                ZXErrStack *err_stack) {
    ZX_ASSERT((err_stack != nullptr));
    ZX_ASSERT((ptr != nullptr));
    ZX_ASSERT((*ptr == nullptr));

    constexpr zx_u16 MAX_ALIGN = 128;
    ZX_ASSERT((align <= MAX_ALIGN));
    ZX_ASSERT((align > 0));
    ZX_ASSERT((zx_is_pow_two_u16(align)));
    ZX_ASSERT((alignof(T) <= align));

    static_assert(sizeof(void *) == sizeof(zx_u64));
    zx_u64 const addr = zx_chk_add_u64((zx_u64)this->buf, this->offset);
    zx_u64 const aligned_addr = zx_next_multiple_of_u64(addr, align);
    zx_u64 const padding = zx_chk_sub_u64(aligned_addr, addr);

    zx_u64 const size = zx_chk_mul_u64(len, sizeof(T));
    zx_u64 const alloc_size = zx_chk_add_u64(padding, size);
    zx_u64 const new_offset = zx_chk_add_u64(this->offset, alloc_size);

    if (new_offset > this->capacity) {
      ZX_ERR_RETURN((ZXStatus::OutOfMemory), err_stack);
    }

    *ptr = (T *)aligned_addr;
    this->offset = new_offset;

    return ZXStatus::Ok;
  }

  template <typename T>
  inline ZXStatus alloc(zx_u64 len, T **ptr, ZXErrStack *err_stack) {
    ZX_ERR_TRY((this->aligned_alloc(alignof(T), len, ptr, err_stack)),
               err_stack);
    return ZXStatus::Ok;
  }
};

#endif
