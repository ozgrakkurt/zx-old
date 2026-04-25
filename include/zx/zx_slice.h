#ifndef ZX_SLICE_H
#define ZX_SLICE_H

#ifndef __LITTLE_ENDIAN__
#error "this code assumes little endian"
#endif

#include <zx/zx_int.h>

static_assert(sizeof(void*) == 8);

enum class [[nodiscard]] ZXSliceCmp : zx_u8_c {
  Equal,
  Greater,
  Less,
};

enum class [[nodiscard]] ZXSliceStatus : zx_u8_c {
  Ok = 0,
  OutOfBounds,
  IntegerOverflow,
  Nullptr,
  UnalignedPtr,
  SizeMismatch,
  AlignmentMismatch,
  Overlapping,
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

#ifdef ZX_DEV
    if (ptr == nullptr) {
      __builtin_trap();
    }

    if (!__builtin_is_aligned(ptr, alignof(T))) {
      __builtin_trap();
    }

    zx_u64 res;
    if (__builtin_mul_overflow(len, zx_u64{sizeof(T)}, &res)) {
      __builtin_trap();
    }
#endif

    return ZXSlice<T>{
        .m_ptr = ptr,
        .m_len = len,
    };
  }

  static ZXSlice<T> init_dangling() {
    static_assert(sizeof(T) > 0);
    static_assert(alignof(T) > 0);
    return ZXSlice<T>{
        .m_ptr = (T const*)zx_u64{alignof(T)},
        .m_len = 0,
    };
  }

  inline zx_u64 len() const { return this->m_len; }

  inline T const* ptr() const { return this->m_ptr; }

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

    if (alignof(U) > alignof(T)) {
      if (!__builtin_is_aligned(this->m_ptr, alignof(U))) {
        return ZXSliceStatus::AlignmentMismatch;
      }
    }

    *out = ZXSlice<U>{.m_ptr = (U const*)this->m_ptr, .m_len = new_len};

    return ZXSliceStatus::Ok;
  }

  template <typename U>
  inline ZXSlice<U> cast_unsafe() const {
    static_assert(sizeof(U) > 0);
    static_assert(alignof(U) > 0);

    zx_u64 const byte_cap = zx_mul_unsafe_u64(this->m_len, sizeof(T));

#ifdef ZX_DEV
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
#ifdef ZX_DEV
    zx_u64 const needed_len = zx_add_unsafe_u64(offset, len);
    if (this->m_len < needed_len) {
      __builtin_trap();
    }
#endif

    return ZXSlice<T>{
        .m_ptr = &this->m_ptr[offset],
        .m_len = len,
    };
  }

  // returns (offset, end]
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

  // returns (offset, end]
  inline ZXSlice<T> slice_at_unsafe(zx_u64 offset) const {
#ifdef ZX_DEV
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
#ifdef ZX_DEV
    if (offset > this->m_len) {
      __builtin_trap();
    }
#endif
    return &this->m_ptr[offset];
  }

  inline ZXSliceStatus read(zx_u64 offset, T* out) const {
    if (out == nullptr) {
      return ZXSliceStatus::Nullptr;
    }

    T const* ptr = nullptr;
    ZXSliceStatus const offset_status = this->offset(offset, &ptr);
    if (ZXSliceStatus::Ok != offset_status) {
      return offset_status;
    }

    *out = *ptr;

    return ZXSliceStatus::Ok;
  }

  inline T read_unsafe(zx_u64 offset) const {
    return *this->offset_unsafe(offset);
  }

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
#ifdef ZX_DEV
    zx_u64 const item_cap = zx_sub_unsafe_u64(this->m_len, index);
    zx_u64 const byte_cap = zx_mul_unsafe_u64(item_cap, sizeof(T));
    if (byte_cap < sizeof(U)) {
      __builtin_trap();
    }
#endif

    U val;
    __builtin_memcpy(&val, &this->m_ptr[index], sizeof(U));
    return val;
  }

  // Returns true iff the given slice is overlapping with `this`.
  // Considers zero sized slices with same pointer to be overlapping.
  inline bool is_overlapping(ZXSlice<T> other) const {
    zx_u64 const self_start = (zx_u64)this->m_ptr;
    zx_u64 const other_start = (zx_u64)other.m_ptr;

    // These additions are ok because the overflow condition is checked in init
    // method.
    zx_u64 const self_end = zx_add_unsafe_u64(self_start, this->m_len);
    zx_u64 const other_end = zx_add_unsafe_u64(other_start, other.m_len);

    return !((self_end < other_start) || (other_end < self_start));
  }

  inline ZXSliceCmp compare(ZXSlice<T> other) const {
    zx_u64 const len = zx_min_u64(this->m_len, other.m_len);
    // This should be ok because overflow condition is checked in the init
    // method.
    zx_u64 const byte_count = zx_mul_unsafe_u64(len, sizeof(T));
    // We are assuming little endian here.
    zx_i32 res = __builtin_memcmp((zx_u8 const*)this->m_ptr,
                                  (zx_u8 const*)other.m_ptr, byte_count);
    if (res == 0) {
      if (this->m_len > other.m_len) {
        return ZXSliceCmp::Greater;
      } else if (this->m_len < other.m_len) {
        return ZXSliceCmp::Less;
      } else {
        return ZXSliceCmp::Equal;
      }
    } else if (res < 0) {
      return ZXSliceCmp::Less;
    } else {
      return ZXSliceCmp::Greater;
    }
  }

  inline bool equals(ZXSlice<T> other) const {
    return this->compare(other) == ZXSliceCmp::Equal;
  }
};

template <typename T>
struct ZXSliceMut {
  zx_u64 m_len;
  T* m_ptr;

  static ZXSliceStatus init(T* ptr, zx_u64 len, ZXSliceMut<T>* out) {
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

    *out = ZXSliceMut<T>{
        .m_ptr = ptr,
        .m_len = len,
    };

    return ZXSliceStatus::Ok;
  }

  static ZXSliceMut<T> init_unsafe(T* ptr, zx_u64 len) {
    static_assert(sizeof(T) > 0);
    static_assert(alignof(T) > 0);

#ifdef ZX_DEV
    if (ptr == nullptr) {
      __builtin_trap();
    }

    if (!__builtin_is_aligned(ptr, alignof(T))) {
      __builtin_trap();
    }

    zx_u64 res;
    if (__builtin_mul_overflow(len, zx_u64{sizeof(T)}, &res)) {
      __builtin_trap();
    }
#endif

    return ZXSliceMut<T>{
        .m_ptr = ptr,
        .m_len = len,
    };
  }

  static ZXSliceMut<T> init_dangling() {
    static_assert(sizeof(T) > 0);
    static_assert(alignof(T) > 0);
    return ZXSliceMut<T>{
        .m_ptr = (T*)zx_u64{alignof(T)},
        .m_len = 0,
    };
  }

  inline zx_u64 len() const { return this->m_len; }

  inline T* ptr() const { return this->m_ptr; }

  template <typename U>
  inline ZXSliceStatus cast(ZXSliceMut<U>* out) const {
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

    if (alignof(U) > alignof(T)) {
      if (!__builtin_is_aligned(this->m_ptr, alignof(U))) {
        return ZXSliceStatus::AlignmentMismatch;
      }
    }

    *out = ZXSliceMut<U>{.m_ptr = (U*)this->m_ptr, .m_len = new_len};

    return ZXSliceStatus::Ok;
  }

  template <typename U>
  inline ZXSliceMut<U> cast_unsafe() const {
    static_assert(sizeof(U) > 0);
    static_assert(alignof(U) > 0);

    zx_u64 const byte_cap = zx_mul_unsafe_u64(this->m_len, sizeof(T));

#ifdef ZX_DEV
    if (byte_cap % sizeof(U) != 0) {
      __builtin_trap();
    }
    if (!__builtin_is_aligned(this->m_ptr, alignof(U))) {
      __builtin_trap();
    }
#endif

    zx_u64 const new_len = zx_div_unsafe_u64(byte_cap, sizeof(U));

    return ZXSliceMut<U>{.m_ptr = (U*)this->m_ptr, .m_len = new_len};
  }

  inline ZXSliceStatus slice(zx_u64 offset,
                             zx_u64 len,
                             ZXSliceMut<T>* out) const {
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

    *out = ZXSliceMut<T>{
        .m_ptr = &this->m_ptr[offset],
        .m_len = len,
    };

    return ZXSliceStatus::Ok;
  }

  inline ZXSliceMut<T> slice_unsafe(zx_u64 offset, zx_u64 len) const {
#ifdef ZX_DEV
    zx_u64 const needed_len = zx_add_unsafe_u64(offset, len);
    if (this->m_len < needed_len) {
      __builtin_trap();
    }
#endif

    return ZXSliceMut<T>{
        .m_ptr = &this->m_ptr[offset],
        .m_len = len,
    };
  }

  // returns (offset, end]
  inline ZXSliceStatus slice_at(zx_u64 offset, ZXSliceMut<T>* out) const {
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

    *out = ZXSliceMut<T>{
        .m_ptr = &this->m_ptr[offset],
        .m_len = len,
    };

    return ZXSliceStatus::Ok;
  }

  // returns (offset, end]
  inline ZXSliceMut<T> slice_at_unsafe(zx_u64 offset) const {
#ifdef ZX_DEV
    if (offset > this->m_len) {
      __builtin_trap();
    }
#endif

    zx_u64 const len = zx_sub_unsafe_u64(this->m_len, offset);

    return ZXSliceMut<T>{
        .m_ptr = &this->m_ptr[offset],
        .m_len = len,
    };
  }

  inline ZXSliceStatus offset(zx_u64 offset, T** out) const {
    if (out == nullptr) {
      return ZXSliceStatus::Nullptr;
    }
    if (offset > this->m_len) {
      return ZXSliceStatus::OutOfBounds;
    }

    *out = &this->m_ptr[offset];

    return ZXSliceStatus::Ok;
  }

  inline T* offset_unsafe(zx_u64 offset) const {
#ifdef ZX_DEV
    if (offset > this->m_len) {
      __builtin_trap();
    }
#endif
    return &this->m_ptr[offset];
  }

  inline ZXSliceStatus write(zx_u64 offset, T val) const {
    T* ptr = nullptr;
    ZXSliceStatus const offset_status = this->offset(offset, &ptr);
    if (ZXSliceStatus::Ok != offset_status) {
      return offset_status;
    }
    *ptr = val;

    return ZXSliceStatus::Ok;
  }

  inline void write_unsafe(zx_u64 offset, T val) const {
    T* ptr = this->offset_unsafe(offset);
    *ptr = val;
  }

  template <typename U>
  inline ZXSliceStatus write_unaligned(zx_u64 index, U val) const {
    bool overflow = false;
    zx_u64 const item_cap = zx_sub_u64(this->m_len, index, &overflow);
    zx_u64 const byte_cap = zx_mul_u64(item_cap, sizeof(T), &overflow);

    if (overflow) {
      return ZXSliceStatus::IntegerOverflow;
    }

    if (byte_cap < sizeof(U)) {
      return ZXSliceStatus::OutOfBounds;
    }

    __builtin_memcpy(&this->m_ptr[index], &val, sizeof(U));
    return ZXSliceStatus::Ok;
  }

  template <typename U>
  inline void write_unaligned_unsafe(zx_u64 index, U val) const {
#ifdef ZX_DEV
    zx_u64 const item_cap = zx_sub_unsafe_u64(this->m_len, index);
    zx_u64 const byte_cap = zx_mul_unsafe_u64(item_cap, sizeof(T));
    if (byte_cap < sizeof(U)) {
      __builtin_trap();
    }
#endif

    __builtin_memcpy(&this->m_ptr[index], &val, sizeof(U));
  }

  inline void fill(T val) const {
    zx_u64 const len = this->m_len;
    T* const ptr = this->m_ptr;
    for (zx_u64 i = 0; i < len; i++) {
      ptr[i] = val;
    }
  }

  inline void memset(zx_u8 byte) const {
    zx_u64 const n_bytes = zx_mul_unsafe_u64(this->m_len, sizeof(T));
    __builtin_memset(this->m_ptr, byte, n_bytes);
  }

  inline ZXSlice<T> as_const() const {
    return ZXSlice<T>{
        .m_ptr = this->m_ptr,
        .m_len = this->m_len,
    };
  }

  inline ZXSliceStatus memcpy_from(ZXSlice<T> other) const {
    if (this->m_len != other.m_len) {
      return ZXSliceStatus::SizeMismatch;
    }

    if (this->as_const().is_overlapping(other.as_const())) {
      return ZXSliceStatus::Overlapping;
    }

    // this is safe to do because overflow condition is checked in the init
    // method.
    zx_u64 const n_bytes = zx_mul_unsafe_u64(this->m_len, sizeof(T));

    __builtin_memcpy(this->m_ptr, other.m_ptr, n_bytes);
    return ZXSliceStatus::Ok;
  }

  inline void memcpy_from_unsafe(ZXSlice<T> other) const {
#ifdef ZX_DEV
    if (this->m_len != other.m_len) {
      __builtin_trap();
    }

    if (this->as_const().is_overlapping(other)) {
      __builtin_trap();
    }
#endif

    // this is safe to do because overflow condition is checked in the init
    // method.
    zx_u64 const n_bytes = zx_mul_unsafe_u64(this->m_len, sizeof(T));

    __builtin_memcpy(this->m_ptr, other.m_ptr, n_bytes);
  }

  inline ZXSliceStatus memmove_from(ZXSlice<T> other) const {
    if (this->m_len != other.m_len) {
      return ZXSliceStatus::SizeMismatch;
    }

    // this is safe to do because overflow condition is checked in the init
    // method.
    zx_u64 const n_bytes = zx_mul_unsafe_u64(this->m_len, sizeof(T));

    __builtin_memmove(this->m_ptr, other.m_ptr, n_bytes);
    return ZXSliceStatus::Ok;
  }

  inline void memmove_from_unsafe(ZXSlice<T> other) const {
#ifdef ZX_DEV
    if (this->m_len != other.m_len) {
      __builtin_trap();
    }
#endif

    // this is safe to do because overflow condition is checked in the init
    // method.
    zx_u64 const n_bytes = zx_mul_unsafe_u64(this->m_len, sizeof(T));

    __builtin_memmove(this->m_ptr, other.m_ptr, n_bytes);
  }
};

struct ZXCString {
  ZXSlice<zx_u8> m_slice;

  static ZXSliceStatus init(char const* cstring, ZXCString* out) {
    if (cstring == nullptr || out == nullptr) {
      return ZXSliceStatus::Nullptr;
    }

    zx_u64 const len = __builtin_strlen(cstring);
    ZXSlice<zx_u8> slice;
    ZXSliceStatus const init_status =
        ZXSlice<zx_u8>::init((zx_u8 const*)cstring, len, &slice);
    if (init_status != ZXSliceStatus::Ok) {
      return init_status;
    }

    *out = ZXCString{
        .m_slice = slice,
    };

    return ZXSliceStatus::Ok;
  }

  static ZXCString init_unsafe(char const* cstring) {
    if (cstring == nullptr) {
#ifdef ZX_DEV
      __builtin_trap();
#else
      return ZXCString{
          .m_slice = ZXSlice<zx_u8>::init_dangling(),
      };
#endif
    }

    zx_u64 const len = __builtin_strlen(cstring);
    return ZXCString{
        .m_slice = ZXSlice<zx_u8>::init_unsafe((zx_u8 const*)cstring, len),
    };
  }

  inline ZXSlice<zx_u8> slice() const { return this->m_slice; }

  inline char const* as_cstr() const {
    return (char const*)this->m_slice.ptr();
  }
};

#endif
