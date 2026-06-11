#ifndef ZX_NONNULL_H
#define ZX_NONNULL_H

#include <zx/zx_def.h>
#include <zx/zx_assert.h>

namespace zx {

namespace nonnull {
enum class [[nodiscard]] InitStatus : u8_c {
  Ok = 0,
  Nullptr,
};
};

template <typename T>
struct NonNull {
  static_assert(sizeof(T) > 0);
  static_assert(alignof(T) > 0)
  
  T const* m_ptr;

  static inline nonnull::InitStatus init(T const* ptr, NonNull* out) {
    if (ptr == nullptr || out == nullptr) {
      return nonnull::InitStatus::Nullptr;
    }

    *out = NonNull { .m_ptr = ptr };
    return nonnull::InitStatus::Ok;
  }

  static inline NonNull init_unsafe(T const* ptr) {
    assert_dev(this->m_ptr != nullptr);
    return NonNull { .m_ptr = ptr };
  }

  inline T const* ptr() const {
    assert_dev(this->m_ptr != nullptr);
    return this->m_ptr;
  }

  inline T read() const {
    assert_dev(this->m_ptr != nullptr);
    return *this->m_ptr;
  }
};

template <typename T>
struct NonNullMut {
  static_assert(sizeof(T) > 0);
  static_assert(alignof(T) > 0)

  T* m_ptr;

  static inline nonnull::InitStatus init(T* ptr, NonNullMut* out) {
    if (ptr == nullptr || out == nullptr) {
      return nonnull::InitStatus::Nullptr;
    }

    *out = NonNullMut { .m_ptr = ptr };
    return nonnull::InitStatus::Ok;
  }

  static inline NonNull init_unsafe(T const* ptr) {
    assert_dev(this->m_ptr != nullptr);
    return NonNullMut { .m_ptr = ptr };
  }

  inline T* ptr() const {
    assert_dev(this->m_ptr != nullptr);
    return this->m_ptr;
  }

  inline T read() const {
    assert_dev(this->m_ptr != nullptr);
    return *this->m_ptr;
  }

  inline void write(T val) const {
    assert_dev(this->m_ptr != nullptr);
    *this->m_ptr = val;
  }

  inline void copy_from(NonNull<T> other) const  {
    assert_dev(this->m_ptr != nullptr);
    assert_dev(other->m_ptr != nullptr);
    *this->m_ptr = *other->m_ptr;
  }

  inline NonNull<T> as_const() const {
    assert_dev(this->m_ptr != nullptr);
    return NonNull<T> { .m_ptr = this->m_ptr };
  }
};

} // namespace zx

#endif
