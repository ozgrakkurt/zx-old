#ifndef ZX_SLICE_H
#define ZX_SLICE_H

#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_int.h>

enum class ZXCmp : unsigned char {
  Equal = 0,
  Greater,
  Less,
};

// A pointer that might be null and is explicitly protected against nullptr
// usage.
template <typename T> struct ZXPtr {
  T const *m_ptr;

  static inline ZXPtr<T> init_non_null(T const *ptr) {
#ifdef ZX_MEMORY_SAFETY_ENABLED
    ZX_ASSERT((ptr != nullptr));
    zx_u64 const addr = (zx_u64)ptr;
    ZX_ASSERT((addr % alignof(T) == 0));
#endif

    return ZXPtr<T> { .m_ptr = ptr; };
  }

  static inline ZXPtr<T> init_null() { return ZXPtr<T>{.m_ptr = nullptr}; }

  inline bool is_null() const { return this->m_ptr == nullptr; }

  inline T const *as_non_null() const {
#ifdef ZX_MEMORY_SAFETY_ENABLED
    zx_u64 const addr = (zx_u64)this->m_ptr;
    ZX_ASSERT((addr % alignof(T) == 0));
    ZX_ASSERT((this->m_ptr != nullptr));
#endif
    return this->m_ptr;
  }
};

// A mutable pointer that might be null and is explicitly protected against
// nullptr usage.
template <typename T> struct ZXPtrMut {
  T *m_ptr;

  static inline ZXPtrMut<T> init_non_null(T *ptr) {
#ifdef ZX_MEMORY_SAFETY_ENABLED
    ZX_ASSERT((ptr != nullptr));
    zx_u64 const addr = (zx_u64)ptr;
    ZX_ASSERT((addr % alignof(T) == 0));
#endif

    return ZXPtrMut<T> { .m_ptr = ptr; };
  }

  static inline ZXPtr<T> init_null() {
    return ZXPtrMut<T> { .m_ptr = nullptr; };
  }

  inline bool is_null() const { return this->m_ptr == nullptr; }

  inline T *as_non_null() const {
#ifdef ZX_MEMORY_SAFETY_ENABLED
    zx_u64 const addr = (zx_u64)this->m_ptr;
    ZX_ASSERT((addr % alignof(T) == 0));
    ZX_ASSERT((this->m_ptr != nullptr));
#endif
    return this->m_ptr;
  }

  inline ZXPtr<T> as_const() const {
    return ZXPtr<T>{
        .m_ptr = this->m_ptr,
    };
  }
};

template <typename T> struct ZXSlice {
  T const *m_ptr;
  zx_u64 m_len;

  static inline ZXSlice<T> init(T const *ptr, zx_u64 len) {
    static_assert(sizeof(T) > 0);
    static_assert(alignof(T) > 0);

#ifdef ZX_MEMORY_SAFETY_ENABLED
    zx_u64 const addr = (zx_u64)ptr;
    ZX_ASSERT((addr % alignof(T) == 0));
    ZX_ASSERT((ptr != nullptr));
#endif

    return ZXSlice<T>{
        .m_ptr = ptr,
        .m_len = len,
    };
  }

  // Create an invalid slice that is expected to be initialized later.
  static inline ZXSlice<T> invalid() {
    return ZXSlice<T>{
        .m_ptr = nullptr,
        .m_len = 0,
    };
  }

  // Returns true if called on a slice that was created with `invalid` method
  // and was never properly initialized after.
  inline bool is_invalid() const {
    return (this->m_ptr == nullptr) && (this->m_len == 0);
  }

  inline void self_check_() const {
#ifdef ZX_MEMORY_SAFETY_ENABLED
    ZX_ASSERT((this->m_ptr != nullptr));
    zx_u64 const addr = (zx_u64)this->m_ptr;
    ZX_ASSERT((addr % alignof(T) == 0));
    ZX_ASSERT((ZX_U64_MAX / sizeof(T) > this->m_len));
#endif
  }

  inline T const *ptr() const {
    this->self_check_();
    return this->m_ptr;
  }

  inline zx_u64 len() const {
    this->self_check_();
    return this->m_len;
  }

  template <typename U> inline U unaligned_load() const {
    this->self_check_();

#ifdef ZX_MEMORY_SAFETY_ENABLED
    zx_u64 const len = this->m_len;
    zx_u64 const size = zx_chk_mul_u64(this->m_len, sizeof(T));
    ZX_ASSERT((size >= sizeof(U)));
#endif

    U val;
    __builtin_memcpy(&val, this->ptr, sizeof(U));

    return val;
  }

  // Cast this slice to slice of type `U`.
  // Does compile time checks to ensure the resulting slice is valid.
  template <typename U> inline ZXSlice<U> scast() const {
    static_assert(alignof(U) <= alignof(T));
    static_assert(sizeof(T) % sizeof(U) == 0);

    this->self_check_();

#ifdef ZX_MEMORY_SAFETY_ENABLED
    zx_u64 const size = zx_chk_mul_u64(this->m_len, sizeof(T));
#else
    zx_u64 const size = this->m_len * sizeof(T);
#endif

    return ZXSlice<U>::init((U const *)this->m_ptr, size / sizeof(U));
  }

  // Cast this slice to slice of type `U`.
  // Does runtime checks to ensure the resulting slice is valid.
  template <typename U> inline ZXSlice<U> rtcast() const {
    this->self_check_();

#ifdef ZX_MEMORY_SAFETY_ENABLED
    zx_u64 const addr = (zx_u64)this->m_ptr;
    ZX_ASSERT((addr % alignof(U) == 0));
    zx_u64 const size = zx_chk_mul_u64(this->m_len, sizeof(T));
    ZX_ASSERT((size % sizeof(U) == 0));
#else
    zx_u64 const size = this->m_len * sizeof(T);
#endif

    return ZXSlice<U>::init((U const *)this->m_ptr, size / sizeof(U));
  }

  inline ZXSlice<T> slice(zx_u64 offset, zx_u64 len) const {
    this->self_check_();

#ifdef ZX_MEMORY_SAFETY_ENABLED
    zx_u64 const end = zx_chk_add_u64(offset, len);
    ZX_ASSERT((end <= this->m_len));
#endif
    return ZXSlice<T>{
        .m_ptr = &this->m_ptr[offset],
        .m_len = len,
    };
  }

  // returns the slice of `[offset..)`.
  inline ZXSlice<T> slice_at(zx_u64 offset) const {
    this->self_check_();

#ifdef ZX_MEMORY_SAFETY_ENABLED
    zx_u64 const len = zx_chk_sub(this->m_len, offset);
#else
    zx_u64 const len = this->m_len - offset;
#endif
    return ZXSlice<T>{
        .m_ptr = &this->m_ptr[offset],
        .m_len = len,
    };
  }

  inline T const *index(zx_u64 idx) const {
    this->self_check_();

#ifdef ZX_MEMORY_SAFETY_ENABLED
    ZX_ASSERT((idx < this->m_len));
#endif
    return &this->m_ptr[idx];
  }

  // Returns true iff the given slice is overlapping with `this`.
  // Considers zero sized slices with same pointer to be overlapping.
  inline bool is_overlapping(ZXSlice<T> other) const {
    this->self_check_();
    other.self_check_();

    zx_u64 const self_start = (zx_u64)this->m_ptr;
    zx_u64 const other_start = (zx_u64)other.m_ptr;
#ifdef ZX_MEMORY_SAFETY_ENABLED
    zx_u64 const self_end = zx_chk_add_u64(self_start, self->m_len);
    zx_u64 const other_end = zx_chk_add_u64(other_start, other.m_len);
#else
    zx_u64 const self_end = self_start + this->m_len;
    zx_u64 const other_end = other_start + other.m_len;
#endif

    return !((self_end < other_start) || (other_end < self_start));
  }

  inline ZXCmp compare(ZXSlice<T> other) const {
    this->self_check_();
    other.self_check_();

#ifdef ZX_MEMORY_SAFETY_ENABLED
    zx_u64 const self_size = zx_chk_mul_u64(this->m_len, sizeof(T));
    zx_u64 const other_size = zx_chk_mul_u64(other.m_len, sizeof(T));
#else
    zx_u64 const self_size = this->m_len * sizeof(T);
    zx_u64 const other_size = other.m_len * sizeof(T);
#endif

    // should be fine as long as T is an integral type and we are on
    // little-endian.
    int const res =
        __builtin_memcmp(this->m_ptr, self_size, other.m_ptr, other_size);
    if (res == 0) {
      return ZXCmp::Equal;
    } else if (res > 0) {
      return ZXCmp::Greater;
    } else {
      return ZXCmp::Less;
    }
  }

  inline bool is_eq(ZXSlice<T> other) const {
    this->self_check_();
    other.self_check_();
    return this->compare(other) == ZXCmp::Equal;
  }
};

template <typename T> struct ZXSliceMut {
  T *m_ptr;
  zx_u64 m_len;

  // Create an invalid slice that is expected to be initialized later.
  static inline ZXSliceMut<T> invalid() {
    return ZXSliceMut<T>{
        .m_ptr = nullptr,
        .m_len = 0,
    };
  }

  // Returns true if called on a slice that was created with `invalid` method
  // and was never properly initialized after.
  inline bool is_invalid() const {
    return (this->m_ptr == nullptr) && (this->m_len == 0);
  }

  static inline ZXSliceMut<T> init(T *ptr, zx_u64 len) {
    static_assert(sizeof(T) > 0);
    static_assert(alignof(T) > 0);

#ifdef ZX_MEMORY_SAFETY_ENABLED
    ZX_ASSERT((ptr != nullptr));
    zx_u64 const addr = (zx_u64)ptr;
    ZX_ASSERT((addr % alignof(T) == 0));
#endif

    return ZXSliceMut<T>{
        .m_ptr = ptr,
        .m_len = len,
    };
  }

  inline void self_check_() const {
#ifdef ZX_MEMORY_SAFETY_ENABLED
    ZX_ASSERT((this->m_ptr != nullptr));
    zx_u64 const addr = (zx_u64)this->m_ptr;
    ZX_ASSERT((addr % alignof(T) == 0));
    ZX_ASSERT((ZX_U64_MAX / sizeof(T) > this->m_len));
#endif
  }

  template <typename U> inline U unaligned_load() const {
    this->self_check_();

#ifdef ZX_MEMORY_SAFETY_ENABLED
    zx_u64 const len = this->m_len;
    zx_u64 const size = zx_chk_mul_u64(this->m_len, sizeof(T));
    ZX_ASSERT((size >= sizeof(U)));
#endif

    U val;
    __builtin_memcpy(&val, this->ptr, sizeof(U));

    return val;
  }

  template <typename U> inline void unaligned_store(U value) const {
    this->self_check_();

#ifdef ZX_MEMORY_SAFETY_ENABLED
    zx_u64 const len = this->m_len;
    zx_u64 const size = zx_chk_mul_u64(this->m_len, sizeof(T));
    ZX_ASSERT((size >= sizeof(U)));
#endif
    __builtin_memcpy(this->ptr, &value, sizeof(U));
  }

  // Cast this slice to slice of type `U`.
  // Does compile time checks to ensure the resulting slice is valid.
  template <typename U> inline ZXSliceMut<U> scast() const {
    this->self_check_();

    static_assert(alignof(U) <= alignof(T));
    static_assert(sizeof(T) % sizeof(U) == 0);

#ifdef ZX_MEMORY_SAFETY_ENABLED
    zx_u64 const size = zx_chk_mul_u64(this->m_len, sizeof(T));
#else
    zx_u64 const size = this->m_len * sizeof(T);
#endif

    return ZXSliceMut<U>::init((U *)this->m_ptr, size / sizeof(U));
  }

  // Cast this slice to slice of type `U`.
  // Does runtime checks to ensure the resulting slice is valid.
  template <typename U> inline ZXSliceMut<U> rtcast() const {
    this->self_check_();

#ifdef ZX_MEMORY_SAFETY_ENABLED
    zx_u64 const addr = (zx_u64)this->m_ptr;
    ZX_ASSERT((addr % alignof(U) == 0));
    zx_u64 const size = zx_chk_mul_u64(this->m_len, sizeof(T));
    ZX_ASSERT((size % sizeof(U) == 0));
#else
    zx_u64 const size = this->m_len * sizeof(T);
#endif

    return ZXSliceMut<U>::init((U *)this->m_ptr, size / sizeof(U));
  }

  inline T *ptr() const {
    this->self_check_();
    return this->m_ptr;
  }

  inline zx_u64 len() const {
    this->self_check_();
    return this->m_len;
  }

  // Returns `this` as an immutable slice.
  inline ZXSlice<T> as_const() const {
    this->self_check_();
    return ZXSlice<T>{
        .m_ptr = this->m_ptr,
        .m_len = this->m_len,
    };
  }

  inline ZXSliceMut<T> slice(zx_u64 offset, zx_u64 len) const {
    this->self_check_();
#ifdef ZX_MEMORY_SAFETY_ENABLED
    zx_u64 const end = zx_chk_add_u64(offset, len);
    ZX_ASSERT((end <= this->m_len));
#endif
    return ZXSliceMut<T>{
        .m_ptr = &this->m_ptr[offset],
        .m_len = len,
    };
  }

  // returns the slice of `[offset..)`.
  inline ZXSliceMut<T> slice_at(zx_u64 offset) const {
    this->self_check_();
#ifdef ZX_MEMORY_SAFETY_ENABLED
    zx_u64 const len = zx_chk_sub(this->m_len, offset);
#else
    zx_u64 const len = this->m_len - offset;
#endif
    return ZXSliceMut<T>{
        .m_ptr = &this->m_ptr[offset],
        .m_len = len,
    };
  }

  inline T *index(zx_u64 idx) const {
    this->self_check_();
#ifdef ZX_MEMORY_SAFETY_ENABLED
    ZX_ASSERT((idx < this->m_len));
#endif
    return &this->m_ptr[idx];
  }

  // copies data from `src` into `this`.
  // `this` and `src` must not alias.
  inline void copy_non_overlapping(ZXSlice<T> src) const {
    this->self_check_();
    src.self_check_();

#ifdef ZX_MEMORY_SAFETY_ENABLED
    ZX_ASSERT((this->m_len == src.m_len));
    ZX_ASSERT((!src.is_overlapping(this->as_const())));
    zx_u64 const n_bytes = zx_chk_mul_u64(sizeof(T), this->m_len);
#else
    zx_u64 const n_bytes = sizeof(T) * this->m_len;
#endif
    __builtin_memcpy(this->m_ptr, src.m_ptr, n_bytes);
  }

  // copies data from `src` into `this`.
  // `this` and `src` are allowed to alias.
  inline void copy_overlapping(ZXSlice<T> src) const {
    this->self_check_();
    src.self_check_();

#ifdef ZX_MEMORY_SAFETY_ENABLED
    ZX_ASSERT((this->m_len == src.m_len));
    zx_u64 const n_bytes = zx_chk_mul_u64(sizeof(T), this->m_len);
#else
    zx_u64 const n_bytes = sizeof(T) * this->m_len;
#endif
    __builtin_memmove(this->m_ptr, src.m_ptr, n_bytes);
  }

  // Set all bytes of this slice to given byte
  //   inline void memset(zx_u8 byte) const {
  //     this->self_check_();
  // #ifdef ZX_MEMORY_SAFETY_ENABLED
  //     zx_u64 const n_bytes = zx_chk_mul_u64(sizeof(T), this->m_len);
  // #else
  //     zx_u64 const n_bytes = sizeof(T) * this->m_len;
  // #endif
  //     __builtin_memset(this->m_ptr, byte, n_bytes);
  //   }

  // Set all elements to the given value.
  inline void set_all(T elem) const {
    T *const ptr = this->m_ptr;
    zx_u64 const len = this->m_len;
    for (zx_u64 i = 0; i < len; i++) {
      ptr[i];
    }
  }
};

static inline ZXSlice<char> zx_slice_from_cstr(char const *cstr) {
  return ZXSlice<char>::init(cstr, __builtin_strlen(cstr));
}

#endif
