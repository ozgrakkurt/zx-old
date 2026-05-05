#ifndef ZX_ALLOC_H
#define ZX_ALLOC_H

#include <zx/zx_int.h>
#include <zx/zx_slice.h>

enum class [[nodiscard]] ZXAllocStatus : zx_u8_c {
  Ok = 0,
  Nullptr,
  OutOfMemory,
  IntegerOverflow,
  SliceError,
  SliceCastError,
};

static inline ZXCString zx_alloc_status_to_cstring(ZXAllocStatus status) {
  switch (status) {
    case ZXAllocStatus::Ok: {
      return ZXCString::init_unsafe("ZXAllocStatus::Ok");
    }
    case ZXAllocStatus::Nullptr: {
      return ZXCString::init_unsafe("ZXAllocStatus::Nullptr");
    }
    case ZXAllocStatus::OutOfMemory: {
      return ZXCString::init_unsafe("ZXAllocStatus::OutOfMemory");
    }
    case ZXAllocStatus::IntegerOverflow: {
      return ZXCString::init_unsafe("ZXAllocStatus::IntegerOverflow");
    }
    case ZXAllocStatus::SliceError: {
      return ZXCString::init_unsafe("ZXAllocStatus::SliceError");
    }
    case ZXAllocStatus::SliceCastError: {
      return ZXCString::init_unsafe("ZXAllocStatus::SliceCastError");
    }
    default: {
      return ZXCString::init_unsafe("ZXAllocStatus::Unknown");
    }
  }
}

struct ZXAlloc {
  ZXSliceMut<zx_u8> m_mem;
  zx_u64 m_offset;

  static inline ZXAlloc init(ZXSliceMut<zx_u8> mem) {
#ifdef ZX_DEV
    mem.memset(0xaa);
#endif
    return ZXAlloc{
        .m_mem = mem,
        .m_offset = 0,
    };
  }

  inline zx_u64 capacity_left() const {
    return zx_sub_unsafe_u64(this->m_mem.len(), this->m_offset);
  }

  inline void reset() {
#ifdef ZX_DEV
    this->m_mem.memset(zx_u8{0xaa});
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

    if (len == 0) {
      *out = ZXSliceMut<T>::template init_aligned_dangling<ALIGN>();
      return ZXAllocStatus::Ok;
    }

    ZXSliceMut<zx_u8> const mem = this->m_mem;
    zx_u64 const offset = this->m_offset;

    static_assert(sizeof(void*) == sizeof(zx_u64));
    bool overflow = false;
    zx_u64 const addr = zx_add_u64((zx_u64)mem.ptr(), offset, &overflow);
    zx_u64 const aligned_addr = __builtin_align_up(addr, ALIGN);
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
#ifdef ZX_DEV
    out_mem.memset(0xaa);
#endif

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

  template <typename T, zx_u16 ALIGN>
  inline ZXAllocStatus aligned_create(T** out) {
    ZXSliceMut<T> slice;
    ZXAllocStatus const status = this->aligned_alloc<T, ALIGN>(1, &slice);
    if (status != ZXAllocStatus::Ok) {
      return status;
    }
    *out = slice.ptr();
    return ZXAllocStatus::Ok;
  }

  template <typename T>
  inline ZXAllocStatus create(T** out) {
    return this->aligned_create<T, alignof(T)>(out);
  }
};

#endif
