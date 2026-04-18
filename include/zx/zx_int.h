#ifndef ZX_INT_H
#define ZX_INT_H

typedef unsigned char zx_u8_c;
static_assert(sizeof(zx_u8_c) == 1);
static_assert(alignof(zx_u8_c) == 1);

typedef unsigned short zx_u16_c;
static_assert(sizeof(zx_u16_c) == 2);
static_assert(alignof(zx_u16_c) == 2);

typedef unsigned int zx_u32_c;
static_assert(sizeof(zx_u32_c) == 4);
static_assert(alignof(zx_u32_c) == 4);

typedef unsigned long zx_u64_c;
static_assert(sizeof(zx_u64_c) == 8);
static_assert(alignof(zx_u64_c) == 8);

typedef char zx_i8_c;
static_assert(sizeof(zx_i8_c) == 1);
static_assert(alignof(zx_i8_c) == 1);

typedef short zx_i16_c;
static_assert(sizeof(zx_i16_c) == 2);
static_assert(alignof(zx_i16_c) == 2);

typedef int zx_i32_c;
static_assert(sizeof(zx_i32_c) == 4);
static_assert(alignof(zx_i32_c) == 4);

typedef long zx_i64_c;
static_assert(sizeof(zx_i64_c) == 8);
static_assert(alignof(zx_i64_c) == 8);

typedef unsigned _BitInt(8) zx_u8;
typedef unsigned _BitInt(16) zx_u16;
typedef unsigned _BitInt(32) zx_u32;
typedef unsigned _BitInt(64) zx_u64;
typedef unsigned _BitInt(128) zx_u128;
typedef unsigned _BitInt(256) zx_u256;
typedef _BitInt(8) zx_i8;
typedef _BitInt(16) zx_i16;
typedef _BitInt(32) zx_i32;
typedef _BitInt(64) zx_i64;
typedef _BitInt(128) zx_i128;
typedef _BitInt(256) zx_i256;

template <typename T>
constexpr static T zx_wrapping_add_(T a, T b) {
  return a + b;
}

template <typename T>
constexpr static T zx_saturating_add_(T a, T b) {
  return __builtin_elementwise_add_sat(a, b);
}

template <typename T>
constexpr static T zx_add_(T a, T b, bool* failure) {
  T res;
  if (__builtin_add_overflow(a, b, &res)) {
    if (failure != nullptr) {
      *failure = true;
    } else {
#ifdef ZX_DEBUG
      __builtin_trap();
#endif
    }
  }
  return res;
}

template <typename T>
constexpr static T zx_add_unsafe_(T a, T b) {
#ifdef ZX_DEBUG
  T res;
  if (__builtin_add_overflow(a, b, &res)) {
    __builtin_trap();
  }
  return res;
#else
  return a + b;
#endif
}

template <typename T>
constexpr static T zx_wrapping_sub_(T a, T b) {
  return a - b;
}

template <typename T>
constexpr static T zx_saturating_sub_(T a, T b) {
  return __builtin_elementwise_sub_sat(a, b);
}

template <typename T>
constexpr static T zx_sub_(T a, T b, bool* failure) {
  T res;
  if (__builtin_sub_overflow(a, b, &res)) {
    if (failure != nullptr) {
      *failure = true;
    } else {
#ifdef ZX_DEBUG
      __builtin_trap();
#endif
    }
  }
  return res;
}

template <typename T>
constexpr static T zx_sub_unsafe_(T a, T b) {
#ifdef ZX_DEBUG
  T res;
  if (__builtin_sub_overflow(a, b, &res)) {
    __builtin_trap();
  }
  return res;
#else
  return a + b;
#endif
}

template <typename T>
constexpr static T zx_wrapping_mul_(T a, T b) {
  return a * b;
}

template <typename T>
constexpr static T zx_saturating_mul_(T a, T b) {
  return __builtin_elementwise_mul_sat(a, b);
}

template <typename T>
constexpr static T zx_mul_(T a, T b, bool* failure) {
  T res;
  if (__builtin_mul_overflow(a, b, &res)) {
    if (failure != nullptr) {
      *failure = true;
    } else {
#ifdef ZX_DEBUG
      __builtin_trap();
#endif
    }
  }
  return res;
}

template <typename T>
constexpr static T zx_mul_unsafe_(T a, T b) {
#ifdef ZX_DEBUG
  T res;
  if (__builtin_mul_overflow(a, b, &res)) {
    __builtin_trap();
  }
  return res;
#else
  return a + b;
#endif
}

template <typename T>
constexpr static T zx_div_(T a, T b, bool* failure) {
  T res;
  if (b == 0) {
    if (failure != nullptr) {
      *failure = true;
    } else {
#ifdef ZX_DEBUG
      __builtin_trap();
#endif
    }
    return 0;
  }
  return a / b;
}

template <typename T>
constexpr static T zx_div_unsafe_(T a, T b) {
  if (b == 0) {
#ifdef ZX_DEBUG
    __builtin_trap();
#else
    return 0;
#endif
  }
  return a / b;
}

template <typename T>
constexpr static T zx_mod_(T a, T b, bool* failure) {
  T res;
  if (b == 0) {
    if (failure != nullptr) {
      *failure = true;
    } else {
#ifdef ZX_DEBUG
      __builtin_trap();
#endif
    }
    return 0;
  }
  return a % b;
}

template <typename T>
constexpr static T zx_mod_unsafe_(T a, T b) {
  if (b == 0) {
#ifdef ZX_DEBUG
    __builtin_trap();
#else
    return 0;
#endif
  }
  return a % b;
}

template <typename T>
constexpr static T zx_next_multiple_of_(T a, T b, bool* failure) {
  T const c = zx_add_<T>(a, 1, failure);
  T const d = zx_div_<T>(c, b, failure);
  return zx_mul_<T>(d, b, failure);
}

#endif
