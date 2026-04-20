#ifndef ZX_SLICE_H
#define ZX_SLICE_H

#include <zx/zx_int.h>

static_assert(sizeof(void*) == 8);

enum class [[nodiscard]] ZXSliceStatus : unsigned char {
  Ok = 0,
  OutOfBounds,
  IntegerOverflow,
  Nullptr,
  UnalignedPtr,
  SizeMismatch,
  AlignmentMismatch,
};

template <typename T>
struct ZXSlice {
  zx_u64 m_len;
  T const* m_ptr;

  static ZXSliceStatus init(T const* ptr, zx_u64 len, ZXSlice<T>* out) {
    static_assert(sizeof(T) > 0);
    static_assert(alignof(T) > 0);

    if (out == nullptr || ptr == nullptr) {
      return ZXSliceStatus::Nullptr;
    }

    if (!__builtin_is_aligned(ptr, alignof(T))) {
      return ZXSliceStatus::UnalignedPtr;
    }

    zx_u64 res;
    if (__builtin_mul_overflow(len, zx_u64{sizeof(T)}, &res)) {
      return ZXSliceStatus::IntegerOverflow;
    }

    *out = ZXSlice<T>{
        .m_ptr = ptr,
        .m_len = len,
    };

    return ZXSliceStatus::Ok;
  }

  static ZXSlice<T> init_unsafe(T const* ptr, zx_u64 len) {
    static_assert(sizeof(T) > 0);
    static_assert(alignof(T) > 0);

#ifdef ZX_DEBUG
    if (ptr == nullptr) {
      __builtin_trap();
    }

    if (!__builtin_is_aligned(ptr, alignof(T))) {
      __builtin_trap();
    }

    zx_u64 res;
    if (__builtin_mul_overflow(len, zx_u64{sizeof(T)}, &res)) {
      return ZXSliceStatus::IntegerOverflow;
    }
#endif

    return ZXSlice<T>{
        .m_ptr = ptr,
        .m_len = len,
    };
  }

  inline zx_u64 len() const { return this->m_len; }

  template <typename U>
  inline ZXSliceStatus read_unaligned(zx_u64 index, U* out) const {
    if (out == nullptr) {
      return ZXSliceStatus::Nullptr;
    }

    bool overflow = false;
    zx_u64 const item_cap = zx_sub_u64(this->m_len, index, &overflow);
    zx_u64 const byte_cap = zx_mul_u64(item_cap, sizeof(T), &overflow);

    if (overflow) {
      __builtin_memset(out, 0, sizeof(U));
      return ZXSliceStatus::IntegerOverflow;
    }

    if (byte_cap < sizeof(U)) {
      __builtin_memset(out, 0, sizeof(U));
      return ZXSliceStatus::OutOfBounds;
    }

    __builtin_memcpy(out, &this->m_ptr[index], sizeof(U));
    return ZXSliceStatus::Ok;
  }

  template <typename U>
  inline U read_unaligned_unsafe(zx_u64 index) const {
    zx_u64 const item_cap = zx_sub_unsafe_u64(this->m_len, index);
    zx_u64 const byte_cap = zx_mul_unsafe_u64(item_cap, sizeof(T));

#ifdef ZX_DEBUG
    if (byte_cap < sizeof(U)) {
      __builtin_trap();
    }
#endif

    U val;
    __builtin_memcpy(&val, &this->m_ptr[index], sizeof(U));
    return val;
  }

  template <typename U>
  inline ZXSliceStatus cast(ZXSlice<U>* out) const {
    static_assert(sizeof(U) > 0);
    static_assert(alignof(U) > 0);

    if (out == nullptr) {
      return ZXSliceStatus::Nullptr;
    }

    bool overflow = false;
    zx_u64 const byte_cap = zx_mul_u64(this->m_len, sizeof(T), &overflow);

    if (overflow) {
      return ZXSliceStatus::IntegerOverflow;
    }

    if (byte_cap % sizeof(U) != 0) {
      return ZXSliceStatus::SizeMismatch;
    }

    overflow = false;
    zx_u64 const new_len = zx_div_u64(byte_cap, sizeof(U), &overflow);

    if (overflow) {
      return ZXSliceStatus::IntegerOverflow;
    }

    if (!__builtin_is_aligned(this->m_ptr, alignof(U))) {
      return ZXSliceStatus::AlignmentMismatch;
    }

    *out = ZXSlice<U>{.m_ptr = (U const*)this->m_ptr, .m_len = new_len};

    return ZXSliceStatus::Ok;
  }

  template <typename U>
  inline ZXSlice<U> cast_unsafe() const {
    static_assert(sizeof(U) > 0);
    static_assert(alignof(U) > 0);

    zx_u64 const byte_cap = zx_mul_unsafe_u64(this->m_len, sizeof(T));

#ifdef ZX_DEBUG
    if (byte_cap % sizeof(U) != 0) {
      __builtin_trap();
    }
    if (!__builtin_is_aligned(this->m_ptr, alignof(U))) {
      __builtin_trap();
    }
#endif

    zx_u64 const new_len = zx_div_unsafe_u64(byte_cap, sizeof(U));

    return ZXSlice<U>{.m_ptr = (U const*)this->m_ptr, .m_len = new_len};
  }

  inline ZXSliceStatus slice(zx_u64 offset, zx_u64 len, ZXSlice<T>* out) const {
    if (out == nullptr) {
      return ZXSliceStatus::Nullptr;
    }
    bool overflow = false;
    zx_u64 const needed_len = zx_add_u64(offset, len, &overflow);
    if (overflow) {
      return ZXSliceStatus::IntegerOverflow;
    }
    if (this->m_len < needed_len) {
      return ZXSliceStatus::OutOfBounds;
    }

    *out = ZXSlice<T>{
        .m_ptr = &this->m_ptr[offset],
        .m_len = len,
    };

    return ZXSliceStatus::Ok;
  }

  inline ZXSlice<T> slice_unsafe(zx_u64 offset, zx_u64 len) const {
    zx_u64 const needed_len = zx_add_unsafe_u64(offset, len);

#ifdef ZX_DEBUG
    if (this->m_len < needed_len) {
      __builtin_trap();
    }
#endif

    return ZXSlice<T>{
        .m_ptr = &this->m_ptr[offset],
        .m_len = len,
    };
  }

  inline ZXSliceStatus slice_at(zx_u64 offset, ZXSlice<T>* out) const {
    if (out == nullptr) {
      return ZXSliceStatus::Nullptr;
    }
    if (offset > this->m_len) {
      return ZXSliceStatus::OutOfBounds;
    }

    bool overflow = false;
    zx_u64 const len = zx_sub_u64(this->m_len, offset, &overflow);
    if (overflow) {
      return ZXSliceStatus::IntegerOverflow;
    }

    *out = ZXSlice<T>{
        .m_ptr = &this->m_ptr[offset],
        .m_len = len,
    };

    return ZXSliceStatus::Ok;
  }

  inline ZXSlice<T> slice_at_unsafe(zx_u64 offset) const {
#ifdef ZX_DEBUG
    if (offset > this->m_len) {
      __builtin_trap();
    }
#endif

    zx_u64 const len = zx_sub_unsafe_u64(this->m_len, offset);

    return ZXSlice<T>{
        .m_ptr = &this->m_ptr[offset],
        .m_len = len,
    };
  }

  inline ZXSliceStatus offset(zx_u64 offset, T const** out) const {
    if (out == nullptr) {
      return ZXSliceStatus::Nullptr;
    }
    if (offset > this->m_len) {
      return ZXSliceStatus::OutOfBounds;
    }

    *out = &this->m_ptr[offset];

    return ZXSliceStatus::Ok;
  }

  inline T const* offset_unsafe(zx_u64 offset) const {
#ifdef ZX_DEBUG
    if (offset > this->m_len) {
      __builtin_trap();
    }
#endif
    return &this->m_ptr[offset];
  }
};

template <typename T>
struct ZXSliceMut {
  zx_u64 m_len;
  T* m_ptr;
};

#endif
