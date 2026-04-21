#ifndef ZX_ALLOC_H
#define ZX_ALLOC_H

#include <zx/zx_int.h>
#include <zx/zx_slice.h>

enum class [[nodiscard]] ZXAllocStatus : zx_u8_c {
  Ok = 0,
  Nullptr,
  OutOfMemory,
  IntegerOverflow,
  InvalidAlignment,
  SliceError,
  SliceCastError,
};

struct ZXAlloc {
  ZXSliceMut<zx_u8> m_mem;
  zx_u64 m_offset;

  static inline ZXAlloc init(ZXSliceMut<zx_u8> mem) {
    return ZXAlloc{
        .m_mem = mem,
        .m_offset = 0,
    };
  }

  inline void reset() {
    this->m_mem.fill(zx_u8{0xaa});
    this->m_offset = 0;
  }

  inline void reset_unsafe() {
#ifdef ZX_DEBUG
    this->m_mem.fill(zx_u8{0xaa});
#endif
    this->m_offset = 0;
  }

  template <typename T, zx_u16 ALIGN>
  inline ZXAllocStatus aligned_alloc(zx_u64 len, ZXSliceMut<T>* out) {
    static_assert(ALIGN >= alignof(T));
    static_assert(ALIGN <= 128);
    static_assert(ALIGN > 0);
    static_assert(zx_is_pow_two_u16(ALIGN));

    if (out == nullptr) {
      return ZXAllocStatus::Nullptr;
    }

    ZXSliceMut<zx_u8> const mem = this->m_mem;
    zx_u64 const offset = this->m_offset;

    static_assert(sizeof(void*) == sizeof(zx_u64));
    bool overflow = false;
    zx_u64 const addr = zx_add_u64((zx_u64)mem.ptr(), offset, &overflow);
    zx_u64 const aligned_addr = zx_next_multiple_of_u64(addr, ALIGN, &overflow);
    zx_u64 const padding = zx_sub_u64(aligned_addr, addr, &overflow);

    zx_u64 const size = zx_mul_u64(len, sizeof(T), &overflow);
    zx_u64 const alloc_size = zx_add_u64(padding, size, &overflow);
    zx_u64 const alloc_offset = zx_add_u64(offset, padding, &overflow);
    zx_u64 const new_offset = zx_add_u64(offset, alloc_size, &overflow);

    if (overflow) {
      return ZXAllocStatus::IntegerOverflow;
    }

    if (new_offset > mem.len()) {
      return ZXAllocStatus::OutOfMemory;
    }

    ZXSliceMut<zx_u8> out_mem;
    if (ZXSliceStatus::Ok != mem.slice(alloc_offset, size, &out_mem)) {
      return ZXAllocStatus::SliceError;
    }

    if (ZXSliceStatus::Ok != out_mem.cast<T>(out)) {
      return ZXAllocStatus::SliceCastError;
    }

    this->m_offset = new_offset;

    return ZXAllocStatus::Ok;
  }

  template <typename T>
  inline ZXAllocStatus alloc(zx_u64 len, ZXSliceMut<T>* out) {
    return this->aligned_alloc<T, alignof(T)>(len, out);
  }
};

#endif
