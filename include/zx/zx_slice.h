#ifndef ZX_SLICE_H
#define ZX_SLICE_H

#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_int.h>

template <typename T> struct ZXSliceMut {
  T *m_ptr;
  zx_u64 m_len;

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
  inline void memcpy_from(ZXSlice<T> src) const {
    this->self_check_();
    src.self_check_();

#ifdef ZX_MEMORY_SAFETY_ENABLED
    ZX_ASSERT((this->m_len == src.len()));
    ZX_ASSERT((!src.is_overlapping(this->as_const())));
    zx_u64 const n_bytes = zx_chk_mul_u64(sizeof(T), this->m_len);
#else
    zx_u64 const n_bytes = sizeof(T) * this->m_len;
#endif
    __builtin_memcpy(this->m_ptr, src.ptr(), n_bytes);
  }

  // copies data from `src` into `this`.
  // `this` and `src` are allowed to alias.
  inline void memmove_from(ZXSlice src) const {
    this->self_check_();
    src.self_check_();

#ifdef ZX_MEMORY_SAFETY_ENABLED
    ZX_ASSERT((this->m_len == src.len()));
    zx_u64 const n_bytes = zx_chk_mul_u64(sizeof(T), this->m_len);
#else
    zx_u64 const n_bytes = sizeof(T) * this->m_len;
#endif
    __builtin_memmove(this->m_ptr, src.ptr(), n_bytes);
  }

  // set all bytes of this slice to given byte
  inline void memset(zx_u8 byte) const {
    this->self_check_();
#ifdef ZX_MEMORY_SAFETY_ENABLED
    zx_u64 const n_bytes = zx_chk_mul_u64(sizeof(T), this->m_len);
#else
    zx_u64 const n_bytes = sizeof(T) * this->m_len;
#endif
    __builtin_memset(this->m_ptr, byte, n_bytes);
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
};

static inline ZXSlice<char> zx_slice_from_cstr(char const *cstr) {
  return ZXSlice<char>::init(cstr, __builtin_strlen(cstr));
}

#endif
