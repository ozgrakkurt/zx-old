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

constexpr zx_u8 ZX_U8_MAX = ~(zx_u8){0};
constexpr zx_u16 ZX_U16_MAX = ~(zx_u16){0};
constexpr zx_u32 ZX_U32_MAX = ~(zx_u32){0};
constexpr zx_u64 ZX_U64_MAX = ~(zx_u64){0};
constexpr zx_u128 ZX_U128_MAX = ~(zx_u128){0};
constexpr zx_u256 ZX_U256_MAX = ~(zx_u256){0};

constexpr zx_i8 ZX_I8_MAX = (zx_i8)(ZX_U8_MAX >> 1);
constexpr zx_i16 ZX_I16_MAX = (zx_i16)(ZX_U16_MAX >> 1);
constexpr zx_i32 ZX_I32_MAX = (zx_i32)(ZX_U32_MAX >> 1);
constexpr zx_i64 ZX_I64_MAX = (zx_i64)(ZX_U64_MAX >> 1);
constexpr zx_i128 ZX_I128_MAX = (zx_i128)(ZX_U128_MAX >> 1);
constexpr zx_i256 ZX_I256_MAX = (zx_i256)(ZX_U256_MAX >> 1);
constexpr zx_i8 ZX_I8_MIN = ~ZX_I8_MAX;
constexpr zx_i16 ZX_I16_MIN = ~ZX_I16_MAX;
constexpr zx_i32 ZX_I32_MIN = ~ZX_I32_MAX;
constexpr zx_i64 ZX_I64_MIN = ~ZX_I64_MAX;
constexpr zx_i128 ZX_I128_MIN = ~ZX_I128_MAX;
constexpr zx_i256 ZX_I256_MIN = ~ZX_I256_MAX;

template <typename T>
constexpr static inline T zx_wrapping_add_(T a, T b) {
  return a + b;
}
constexpr static inline zx_u8 zx_wrapping_add_u8(zx_u8 a, zx_u8 b) {
  return zx_wrapping_add_<zx_u8>(a, b);
}
constexpr static inline zx_u16 zx_wrapping_add_u16(zx_u16 a, zx_u16 b) {
  return zx_wrapping_add_<zx_u16>(a, b);
}
constexpr static inline zx_u32 zx_wrapping_add_u32(zx_u32 a, zx_u32 b) {
  return zx_wrapping_add_<zx_u32>(a, b);
}
constexpr static inline zx_u64 zx_wrapping_add_u64(zx_u64 a, zx_u64 b) {
  return zx_wrapping_add_<zx_u64>(a, b);
}

template <typename T>
constexpr static inline T zx_saturating_add_(T a, T b) {
  return __builtin_elementwise_add_sat(a, b);
}
constexpr static inline zx_u8 zx_saturating_add_u8(zx_u8 a, zx_u8 b) {
  return zx_saturating_add_<zx_u8>(a, b);
}
constexpr static inline zx_u16 zx_saturating_add_u16(zx_u16 a, zx_u16 b) {
  return zx_saturating_add_<zx_u16>(a, b);
}
constexpr static inline zx_u32 zx_saturating_add_u32(zx_u32 a, zx_u32 b) {
  return zx_saturating_add_<zx_u32>(a, b);
}
constexpr static inline zx_u64 zx_saturating_add_u64(zx_u64 a, zx_u64 b) {
  return zx_saturating_add_<zx_u64>(a, b);
}
constexpr static inline zx_i8 zx_saturating_add_i8(zx_i8 a, zx_i8 b) {
  return zx_saturating_add_<zx_i8>(a, b);
}
constexpr static inline zx_i16 zx_saturating_add_i16(zx_i16 a, zx_i16 b) {
  return zx_saturating_add_<zx_i16>(a, b);
}
constexpr static inline zx_i32 zx_saturating_add_i32(zx_i32 a, zx_i32 b) {
  return zx_saturating_add_<zx_i32>(a, b);
}
constexpr static inline zx_i64 zx_saturating_add_i64(zx_i64 a, zx_i64 b) {
  return zx_saturating_add_<zx_i64>(a, b);
}

template <typename T>
constexpr static inline T zx_add_(T a, T b, bool* failure) {
  T res;
  if (__builtin_add_overflow(a, b, &res)) [[unlikely]] {
    if (failure != nullptr) [[likely]] {
      *failure = true;
    } else {
#ifdef ZX_DEBUG
      __builtin_trap();
#endif
    }
  }
  return res;
}
constexpr static inline zx_u8 zx_add_u8(zx_u8 a, zx_u8 b, bool* failure) {
  return zx_add_<zx_u8>(a, b, failure);
}
constexpr static inline zx_u16 zx_add_u16(zx_u16 a, zx_u16 b, bool* failure) {
  return zx_add_<zx_u16>(a, b, failure);
}
constexpr static inline zx_u32 zx_add_u32(zx_u32 a, zx_u32 b, bool* failure) {
  return zx_add_<zx_u32>(a, b, failure);
}
constexpr static inline zx_u64 zx_add_u64(zx_u64 a, zx_u64 b, bool* failure) {
  return zx_add_<zx_u64>(a, b, failure);
}
constexpr static inline zx_i8 zx_add_i8(zx_i8 a, zx_i8 b, bool* failure) {
  return zx_add_<zx_i8>(a, b, failure);
}
constexpr static inline zx_i16 zx_add_i16(zx_i16 a, zx_i16 b, bool* failure) {
  return zx_add_<zx_i16>(a, b, failure);
}
constexpr static inline zx_i32 zx_add_i32(zx_i32 a, zx_i32 b, bool* failure) {
  return zx_add_<zx_i32>(a, b, failure);
}
constexpr static inline zx_i64 zx_add_i64(zx_i64 a, zx_i64 b, bool* failure) {
  return zx_add_<zx_i64>(a, b, failure);
}

template <typename T>
constexpr static inline T zx_add_unsafe_(T a, T b) {
#ifdef ZX_DEBUG
  T res;
  if (__builtin_add_overflow(a, b, &res)) [[unlikely]] {
    __builtin_trap();
  }
  return res;
#else
  return a + b;
#endif
}
constexpr static inline zx_u8 zx_add_unsafe_u8(zx_u8 a, zx_u8 b) {
  return zx_add_unsafe_<zx_u8>(a, b);
}
constexpr static inline zx_u16 zx_add_unsafe_u16(zx_u16 a, zx_u16 b) {
  return zx_add_unsafe_<zx_u16>(a, b);
}
constexpr static inline zx_u32 zx_add_unsafe_u32(zx_u32 a, zx_u32 b) {
  return zx_add_unsafe_<zx_u32>(a, b);
}
constexpr static inline zx_u64 zx_add_unsafe_u64(zx_u64 a, zx_u64 b) {
  return zx_add_unsafe_<zx_u64>(a, b);
}
constexpr static inline zx_i8 zx_add_unsafe_i8(zx_i8 a, zx_i8 b) {
  return zx_add_unsafe_<zx_i8>(a, b);
}
constexpr static inline zx_i16 zx_add_unsafe_i16(zx_i16 a, zx_i16 b) {
  return zx_add_unsafe_<zx_i16>(a, b);
}
constexpr static inline zx_i32 zx_add_unsafe_i32(zx_i32 a, zx_i32 b) {
  return zx_add_unsafe_<zx_i32>(a, b);
}
constexpr static inline zx_i64 zx_add_unsafe_i64(zx_i64 a, zx_i64 b) {
  return zx_add_unsafe_<zx_i64>(a, b);
}

template <typename T>
constexpr static inline T zx_wrapping_sub_(T a, T b) {
  return a - b;
}
constexpr static inline zx_u8 zx_wrapping_sub_u8(zx_u8 a, zx_u8 b) {
  return zx_wrapping_sub_<zx_u8>(a, b);
}
constexpr static inline zx_u16 zx_wrapping_sub_u16(zx_u16 a, zx_u16 b) {
  return zx_wrapping_sub_<zx_u16>(a, b);
}
constexpr static inline zx_u32 zx_wrapping_sub_u32(zx_u32 a, zx_u32 b) {
  return zx_wrapping_sub_<zx_u32>(a, b);
}
constexpr static inline zx_u64 zx_wrapping_sub_u64(zx_u64 a, zx_u64 b) {
  return zx_wrapping_sub_<zx_u64>(a, b);
}

template <typename T>
constexpr static inline T zx_saturating_sub_(T a, T b) {
  return __builtin_elementwise_sub_sat(a, b);
}
constexpr static inline zx_u8 zx_saturating_sub_u8(zx_u8 a, zx_u8 b) {
  return zx_saturating_sub_<zx_u8>(a, b);
}
constexpr static inline zx_u16 zx_saturating_sub_u16(zx_u16 a, zx_u16 b) {
  return zx_saturating_sub_<zx_u16>(a, b);
}
constexpr static inline zx_u32 zx_saturating_sub_u32(zx_u32 a, zx_u32 b) {
  return zx_saturating_sub_<zx_u32>(a, b);
}
constexpr static inline zx_u64 zx_saturating_sub_u64(zx_u64 a, zx_u64 b) {
  return zx_saturating_sub_<zx_u64>(a, b);
}
constexpr static inline zx_i8 zx_saturating_sub_i8(zx_i8 a, zx_i8 b) {
  return zx_saturating_sub_<zx_i8>(a, b);
}
constexpr static inline zx_i16 zx_saturating_sub_i16(zx_i16 a, zx_i16 b) {
  return zx_saturating_sub_<zx_i16>(a, b);
}
constexpr static inline zx_i32 zx_saturating_sub_i32(zx_i32 a, zx_i32 b) {
  return zx_saturating_sub_<zx_i32>(a, b);
}
constexpr static inline zx_i64 zx_saturating_sub_i64(zx_i64 a, zx_i64 b) {
  return zx_saturating_sub_<zx_i64>(a, b);
}

template <typename T>
constexpr static inline T zx_sub_(T a, T b, bool* failure) {
  T res;
  if (__builtin_sub_overflow(a, b, &res)) [[unlikely]] {
    if (failure != nullptr) [[likely]] {
      *failure = true;
    } else {
#ifdef ZX_DEBUG
      __builtin_trap();
#endif
    }
  }
  return res;
}
constexpr static inline zx_u8 zx_sub_u8(zx_u8 a, zx_u8 b, bool* failure) {
  return zx_sub_<zx_u8>(a, b, failure);
}
constexpr static inline zx_u16 zx_sub_u16(zx_u16 a, zx_u16 b, bool* failure) {
  return zx_sub_<zx_u16>(a, b, failure);
}
constexpr static inline zx_u32 zx_sub_u32(zx_u32 a, zx_u32 b, bool* failure) {
  return zx_sub_<zx_u32>(a, b, failure);
}
constexpr static inline zx_u64 zx_sub_u64(zx_u64 a, zx_u64 b, bool* failure) {
  return zx_sub_<zx_u64>(a, b, failure);
}
constexpr static inline zx_i8 zx_sub_i8(zx_i8 a, zx_i8 b, bool* failure) {
  return zx_sub_<zx_i8>(a, b, failure);
}
constexpr static inline zx_i16 zx_sub_i16(zx_i16 a, zx_i16 b, bool* failure) {
  return zx_sub_<zx_i16>(a, b, failure);
}
constexpr static inline zx_i32 zx_sub_i32(zx_i32 a, zx_i32 b, bool* failure) {
  return zx_sub_<zx_i32>(a, b, failure);
}
constexpr static inline zx_i64 zx_sub_i64(zx_i64 a, zx_i64 b, bool* failure) {
  return zx_sub_<zx_i64>(a, b, failure);
}

template <typename T>
constexpr static inline T zx_sub_unsafe_(T a, T b) {
#ifdef ZX_DEBUG
  T res;
  if (__builtin_sub_overflow(a, b, &res)) [[unlikely]] {
    __builtin_trap();
  }
  return res;
#else
  return a + b;
#endif
}
constexpr static inline zx_u8 zx_sub_unsafe_u8(zx_u8 a, zx_u8 b) {
  return zx_sub_unsafe_<zx_u8>(a, b);
}
constexpr static inline zx_u16 zx_sub_unsafe_u16(zx_u16 a, zx_u16 b) {
  return zx_sub_unsafe_<zx_u16>(a, b);
}
constexpr static inline zx_u32 zx_sub_unsafe_u32(zx_u32 a, zx_u32 b) {
  return zx_sub_unsafe_<zx_u32>(a, b);
}
constexpr static inline zx_u64 zx_sub_unsafe_u64(zx_u64 a, zx_u64 b) {
  return zx_sub_unsafe_<zx_u64>(a, b);
}
constexpr static inline zx_i8 zx_sub_unsafe_i8(zx_i8 a, zx_i8 b) {
  return zx_sub_unsafe_<zx_i8>(a, b);
}
constexpr static inline zx_i16 zx_sub_unsafe_i16(zx_i16 a, zx_i16 b) {
  return zx_sub_unsafe_<zx_i16>(a, b);
}
constexpr static inline zx_i32 zx_sub_unsafe_i32(zx_i32 a, zx_i32 b) {
  return zx_sub_unsafe_<zx_i32>(a, b);
}
constexpr static inline zx_i64 zx_sub_unsafe_i64(zx_i64 a, zx_i64 b) {
  return zx_sub_unsafe_<zx_i64>(a, b);
}

template <typename T>
constexpr static inline T zx_wrapping_mul_(T a, T b) {
  return a * b;
}
constexpr static inline zx_u8 zx_wrapping_mul_u8(zx_u8 a, zx_u8 b) {
  return zx_wrapping_mul_<zx_u8>(a, b);
}
constexpr static inline zx_u16 zx_wrapping_mul_u16(zx_u16 a, zx_u16 b) {
  return zx_wrapping_mul_<zx_u16>(a, b);
}
constexpr static inline zx_u32 zx_wrapping_mul_u32(zx_u32 a, zx_u32 b) {
  return zx_wrapping_mul_<zx_u32>(a, b);
}
constexpr static inline zx_u64 zx_wrapping_mul_u64(zx_u64 a, zx_u64 b) {
  return zx_wrapping_mul_<zx_u64>(a, b);
}

template <typename T>
constexpr static inline T zx_mul_(T a, T b, bool* failure) {
  T res;
  if (__builtin_mul_overflow(a, b, &res)) [[unlikely]] {
    if (failure != nullptr) [[likely]] {
      *failure = true;
    } else {
#ifdef ZX_DEBUG
      __builtin_trap();
#endif
    }
  }
  return res;
}
constexpr static inline zx_u8 zx_mul_u8(zx_u8 a, zx_u8 b, bool* failure) {
  return zx_mul_<zx_u8>(a, b, failure);
}
constexpr static inline zx_u16 zx_mul_u16(zx_u16 a, zx_u16 b, bool* failure) {
  return zx_mul_<zx_u16>(a, b, failure);
}
constexpr static inline zx_u32 zx_mul_u32(zx_u32 a, zx_u32 b, bool* failure) {
  return zx_mul_<zx_u32>(a, b, failure);
}
constexpr static inline zx_u64 zx_mul_u64(zx_u64 a, zx_u64 b, bool* failure) {
  return zx_mul_<zx_u64>(a, b, failure);
}
constexpr static inline zx_i8 zx_mul_i8(zx_i8 a, zx_i8 b, bool* failure) {
  return zx_mul_<zx_i8>(a, b, failure);
}
constexpr static inline zx_i16 zx_mul_i16(zx_i16 a, zx_i16 b, bool* failure) {
  return zx_mul_<zx_i16>(a, b, failure);
}
constexpr static inline zx_i32 zx_mul_i32(zx_i32 a, zx_i32 b, bool* failure) {
  return zx_mul_<zx_i32>(a, b, failure);
}
constexpr static inline zx_i64 zx_mul_i64(zx_i64 a, zx_i64 b, bool* failure) {
  return zx_mul_<zx_i64>(a, b, failure);
}

template <typename T>
constexpr static inline T zx_mul_unsafe_(T a, T b) {
#ifdef ZX_DEBUG
  T res;
  if (__builtin_mul_overflow(a, b, &res)) [[unlikely]] {
    __builtin_trap();
  }
  return res;
#else
  return a + b;
#endif
}
constexpr static inline zx_u8 zx_mul_unsafe_u8(zx_u8 a, zx_u8 b) {
  return zx_mul_unsafe_<zx_u8>(a, b);
}
constexpr static inline zx_u16 zx_mul_unsafe_u16(zx_u16 a, zx_u16 b) {
  return zx_mul_unsafe_<zx_u16>(a, b);
}
constexpr static inline zx_u32 zx_mul_unsafe_u32(zx_u32 a, zx_u32 b) {
  return zx_mul_unsafe_<zx_u32>(a, b);
}
constexpr static inline zx_u64 zx_mul_unsafe_u64(zx_u64 a, zx_u64 b) {
  return zx_mul_unsafe_<zx_u64>(a, b);
}
constexpr static inline zx_i8 zx_mul_unsafe_i8(zx_i8 a, zx_i8 b) {
  return zx_mul_unsafe_<zx_i8>(a, b);
}
constexpr static inline zx_i16 zx_mul_unsafe_i16(zx_i16 a, zx_i16 b) {
  return zx_mul_unsafe_<zx_i16>(a, b);
}
constexpr static inline zx_i32 zx_mul_unsafe_i32(zx_i32 a, zx_i32 b) {
  return zx_mul_unsafe_<zx_i32>(a, b);
}
constexpr static inline zx_i64 zx_mul_unsafe_i64(zx_i64 a, zx_i64 b) {
  return zx_mul_unsafe_<zx_i64>(a, b);
}

template <typename T>
constexpr static inline T zx_div_(T a, T b, bool* failure) {
  if (b == 0) [[unlikely]] {
    if (failure != nullptr) [[likely]] {
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
constexpr static inline zx_u8 zx_div_u8(zx_u8 a, zx_u8 b, bool* failure) {
  return zx_div_<zx_u8>(a, b, failure);
}
constexpr static inline zx_u16 zx_div_u16(zx_u16 a, zx_u16 b, bool* failure) {
  return zx_div_<zx_u16>(a, b, failure);
}
constexpr static inline zx_u32 zx_div_u32(zx_u32 a, zx_u32 b, bool* failure) {
  return zx_div_<zx_u32>(a, b, failure);
}
constexpr static inline zx_u64 zx_div_u64(zx_u64 a, zx_u64 b, bool* failure) {
  return zx_div_<zx_u64>(a, b, failure);
}

template <typename T>
constexpr static inline T zx_div_unsafe_(T a, T b) {
  if (b == 0) [[unlikely]] {
#ifdef ZX_DEBUG
    __builtin_trap();
#else
    return 0;
#endif
  }
  return a / b;
}
constexpr static inline zx_u8 zx_div_unsafe_u8(zx_u8 a, zx_u8 b) {
  return zx_div_unsafe_<zx_u8>(a, b);
}
constexpr static inline zx_u16 zx_div_unsafe_u16(zx_u16 a, zx_u16 b) {
  return zx_div_unsafe_<zx_u16>(a, b);
}
constexpr static inline zx_u32 zx_div_unsafe_u32(zx_u32 a, zx_u32 b) {
  return zx_div_unsafe_<zx_u32>(a, b);
}
constexpr static inline zx_u64 zx_div_unsafe_u64(zx_u64 a, zx_u64 b) {
  return zx_div_unsafe_<zx_u64>(a, b);
}

template <typename T>
constexpr static inline T zx_mod_(T a, T b, bool* failure) {
  if (b == 0) [[unlikely]] {
    if (failure != nullptr) [[likely]] {
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
constexpr static inline zx_u8 zx_mod_u8(zx_u8 a, zx_u8 b, bool* failure) {
  return zx_mod_<zx_u8>(a, b, failure);
}
constexpr static inline zx_u16 zx_mod_u16(zx_u16 a, zx_u16 b, bool* failure) {
  return zx_mod_<zx_u16>(a, b, failure);
}
constexpr static inline zx_u32 zx_mod_u32(zx_u32 a, zx_u32 b, bool* failure) {
  return zx_mod_<zx_u32>(a, b, failure);
}
constexpr static inline zx_u64 zx_mod_u64(zx_u64 a, zx_u64 b, bool* failure) {
  return zx_mod_<zx_u64>(a, b, failure);
}

template <typename T>
constexpr static inline T zx_mod_unsafe_(T a, T b) {
  if (b == 0) [[unlikely]] {
#ifdef ZX_DEBUG
    __builtin_trap();
#else
    return 0;
#endif
  }
  return a % b;
}
constexpr static inline zx_u8 zx_mod_unsafe_u8(zx_u8 a, zx_u8 b) {
  return zx_mod_unsafe_<zx_u8>(a, b);
}
constexpr static inline zx_u16 zx_mod_unsafe_u16(zx_u16 a, zx_u16 b) {
  return zx_mod_unsafe_<zx_u16>(a, b);
}
constexpr static inline zx_u32 zx_mod_unsafe_u32(zx_u32 a, zx_u32 b) {
  return zx_mod_unsafe_<zx_u32>(a, b);
}
constexpr static inline zx_u64 zx_mod_unsafe_u64(zx_u64 a, zx_u64 b) {
  return zx_mod_unsafe_<zx_u64>(a, b);
}

template <typename T>
constexpr static inline T zx_next_multiple_of_(T a, T b, bool* failure) {
  T const c = zx_add_<T>(a, b, failure);
  T const d = zx_sub_<T>(c, 1, failure);
  T const e = zx_div_<T>(d, b, failure);
  return zx_mul_<T>(e, b, failure);
}
constexpr static inline zx_u8 zx_next_multiple_of_u8(zx_u8 a,
                                                     zx_u8 b,
                                                     bool* failure) {
  return zx_next_multiple_of_<zx_u8>(a, b, failure);
}
constexpr static inline zx_u16 zx_next_multiple_of_u16(zx_u16 a,
                                                       zx_u16 b,
                                                       bool* failure) {
  return zx_next_multiple_of_<zx_u16>(a, b, failure);
}
constexpr static inline zx_u32 zx_next_multiple_of_u32(zx_u32 a,
                                                       zx_u32 b,
                                                       bool* failure) {
  return zx_next_multiple_of_<zx_u32>(a, b, failure);
}
constexpr static inline zx_u64 zx_next_multiple_of_u64(zx_u64 a,
                                                       zx_u64 b,
                                                       bool* failure) {
  return zx_next_multiple_of_<zx_u64>(a, b, failure);
}

template <typename T>
constexpr static inline T zx_next_multiple_of_unsafe_(T a, T b) {
  T const c = zx_add_unsafe_<T>(a, 1);
  T const d = zx_div_unsafe_<T>(c, b);
  return zx_mul_unsafe_<T>(d, b);
}
constexpr static inline zx_u8 zx_next_multiple_of_unsafe_u8(zx_u8 a, zx_u8 b) {
  return zx_next_multiple_of_unsafe_<zx_u8>(a, b);
}
constexpr static inline zx_u16 zx_next_multiple_of_unsafe_u16(zx_u16 a,
                                                              zx_u16 b) {
  return zx_next_multiple_of_unsafe_<zx_u16>(a, b);
}
constexpr static inline zx_u32 zx_next_multiple_of_unsafe_u32(zx_u32 a,
                                                              zx_u32 b) {
  return zx_next_multiple_of_unsafe_<zx_u32>(a, b);
}
constexpr static inline zx_u64 zx_next_multiple_of_unsafe_u64(zx_u64 a,
                                                              zx_u64 b) {
  return zx_next_multiple_of_unsafe_<zx_u64>(a, b);
}

template <typename T>
constexpr static inline T zx_min_(T a, T b) {
  return (a > b) ? b : a;
}
constexpr static inline zx_u8 zx_min_u8(zx_u8 a, zx_u8 b) {
  return zx_min_<zx_u8>(a, b);
}
constexpr static inline zx_u16 zx_min_u16(zx_u16 a, zx_u16 b) {
  return zx_min_<zx_u16>(a, b);
}
constexpr static inline zx_u32 zx_min_u32(zx_u32 a, zx_u32 b) {
  return zx_min_<zx_u32>(a, b);
}
constexpr static inline zx_u64 zx_min_u64(zx_u64 a, zx_u64 b) {
  return zx_min_<zx_u64>(a, b);
}
constexpr static inline zx_i8 zx_min_i8(zx_i8 a, zx_i8 b) {
  return zx_min_<zx_i8>(a, b);
}
constexpr static inline zx_i16 zx_min_i16(zx_i16 a, zx_i16 b) {
  return zx_min_<zx_i16>(a, b);
}
constexpr static inline zx_i32 zx_min_i32(zx_i32 a, zx_i32 b) {
  return zx_min_<zx_i32>(a, b);
}
constexpr static inline zx_i64 zx_min_i64(zx_i64 a, zx_i64 b) {
  return zx_min_<zx_i64>(a, b);
}

template <typename T>
constexpr static inline T zx_max_(T a, T b) {
  return (a < b) ? b : a;
}
constexpr static inline zx_u8 zx_max_u8(zx_u8 a, zx_u8 b) {
  return zx_max_<zx_u8>(a, b);
}
constexpr static inline zx_u16 zx_max_u16(zx_u16 a, zx_u16 b) {
  return zx_max_<zx_u16>(a, b);
}
constexpr static inline zx_u32 zx_max_u32(zx_u32 a, zx_u32 b) {
  return zx_max_<zx_u32>(a, b);
}
constexpr static inline zx_u64 zx_max_u64(zx_u64 a, zx_u64 b) {
  return zx_max_<zx_u64>(a, b);
}
constexpr static inline zx_i8 zx_max_i8(zx_i8 a, zx_i8 b) {
  return zx_max_<zx_i8>(a, b);
}
constexpr static inline zx_i16 zx_max_i16(zx_i16 a, zx_i16 b) {
  return zx_max_<zx_i16>(a, b);
}
constexpr static inline zx_i32 zx_max_i32(zx_i32 a, zx_i32 b) {
  return zx_max_<zx_i32>(a, b);
}
constexpr static inline zx_i64 zx_max_i64(zx_i64 a, zx_i64 b) {
  return zx_max_<zx_i64>(a, b);
}

constexpr static inline zx_u8 zx_ctz_u8(zx_u8 a) {
  return (zx_u8)__builtin_ctzg(a, 8);
}
constexpr static inline zx_u8 zx_ctz_u16(zx_u16 a) {
  return (zx_u8)__builtin_ctzg(a, 16);
}
constexpr static inline zx_u8 zx_ctz_u32(zx_u32 a) {
  return (zx_u8)__builtin_ctzg(a, 32);
}
constexpr static inline zx_u8 zx_ctz_u64(zx_u64 a) {
  return (zx_u8)__builtin_ctzg(a, 64);
}

constexpr static inline zx_u8 zx_clz_u8(zx_u8 a) {
  return (zx_u8)__builtin_clzg(a, 8);
}
constexpr static inline zx_u8 zx_clz_u16(zx_u16 a) {
  return (zx_u8)__builtin_clzg(a, 16);
}
constexpr static inline zx_u8 zx_clz_u32(zx_u32 a) {
  return (zx_u8)__builtin_clzg(a, 32);
}
constexpr static inline zx_u8 zx_clz_u64(zx_u64 a) {
  return (zx_u8)__builtin_clzg(a, 64);
}

constexpr static inline zx_u8 zx_popcnt_u8(zx_u8 a) {
  return (zx_u8)__builtin_popcountg(a);
}
constexpr static inline zx_u8 zx_popcnt_u16(zx_u16 a) {
  return (zx_u8)__builtin_popcountg(a);
}
constexpr static inline zx_u8 zx_popcnt_u32(zx_u32 a) {
  return (zx_u8)__builtin_popcountg(a);
}
constexpr static inline zx_u8 zx_popcnt_u64(zx_u64 a) {
  return (zx_u8)__builtin_popcountg(a);
}

template <typename T>
constexpr static inline zx_u8 zx_bit_width_(T a) {
  static_assert(sizeof(T) <= 8);
  constexpr zx_u8 N_BITS = sizeof(T) * 8;
  return N_BITS - (zx_u8)__builtin_clzg(a, int{N_BITS});
}
constexpr static inline zx_u8 zx_bit_width_u8(zx_u8 a) {
  return zx_bit_width_<zx_u8>(a);
}
constexpr static inline zx_u8 zx_bit_width_u16(zx_u16 a) {
  return zx_bit_width_<zx_u16>(a);
}
constexpr static inline zx_u8 zx_bit_width_u32(zx_u32 a) {
  return zx_bit_width_<zx_u32>(a);
}
constexpr static inline zx_u8 zx_bit_width_u64(zx_u64 a) {
  return zx_bit_width_<zx_u64>(a);
}

template <typename T>
constexpr static inline T zx_shl_(T a, zx_u8 amount, bool* failure) {
  constexpr zx_u8 N_BITS = sizeof(T) * 8;
  if (amount >= N_BITS) [[unlikely]] {
    if (failure != nullptr) [[likely]] {
      *failure = true;
    } else {
#ifdef ZX_DEBUG
      __builtin_trap();
#endif
    }
    return 0;
  }
  return a << amount;
}
constexpr static inline zx_u8 zx_shl_u8(zx_u8 a, zx_u8 amount, bool* failure) {
  return zx_shl_<zx_u8>(a, amount, failure);
}
constexpr static inline zx_u16 zx_shl_u16(zx_u16 a,
                                          zx_u8 amount,
                                          bool* failure) {
  return zx_shl_<zx_u16>(a, amount, failure);
}
constexpr static inline zx_u32 zx_shl_u32(zx_u32 a,
                                          zx_u8 amount,
                                          bool* failure) {
  return zx_shl_<zx_u32>(a, amount, failure);
}
constexpr static inline zx_u64 zx_shl_u64(zx_u64 a,
                                          zx_u8 amount,
                                          bool* failure) {
  return zx_shl_<zx_u64>(a, amount, failure);
}

template <typename T>
constexpr static inline T zx_shl_unsafe_(T a, zx_u8 amount) {
  constexpr zx_u8 N_BITS = sizeof(T) * 8;
  if (amount >= N_BITS) [[unlikely]] {
#ifdef ZX_DEBUG
    __builtin_trap();
#else
    return 0;
#endif
  }
  return a << amount;
}
constexpr static inline zx_u8 zx_shl_unsafe_u8(zx_u8 a, zx_u8 amount) {
  return zx_shl_unsafe_<zx_u8>(a, amount);
}
constexpr static inline zx_u16 zx_shl_unsafe_u16(zx_u16 a, zx_u8 amount) {
  return zx_shl_unsafe_<zx_u16>(a, amount);
}
constexpr static inline zx_u32 zx_shl_unsafe_u32(zx_u32 a, zx_u8 amount) {
  return zx_shl_unsafe_<zx_u32>(a, amount);
}
constexpr static inline zx_u64 zx_shl_unsafe_u64(zx_u64 a, zx_u8 amount) {
  return zx_shl_unsafe_<zx_u64>(a, amount);
}

template <typename T>
constexpr static inline T zx_shr_(T a, zx_u8 amount, bool* failure) {
  constexpr zx_u8 N_BITS = sizeof(T) * 8;
  if (amount >= N_BITS) [[unlikely]] {
    if (failure != nullptr) [[likely]] {
      *failure = true;
    } else {
#ifdef ZX_DEBUG
      __builtin_trap();
#endif
    }
    return 0;
  }
  return a >> amount;
}
constexpr static inline zx_u8 zx_shr_u8(zx_u8 a, zx_u8 amount, bool* failure) {
  return zx_shr_<zx_u8>(a, amount, failure);
}
constexpr static inline zx_u16 zx_shr_u16(zx_u16 a,
                                          zx_u8 amount,
                                          bool* failure) {
  return zx_shr_<zx_u16>(a, amount, failure);
}
constexpr static inline zx_u32 zx_shr_u32(zx_u32 a,
                                          zx_u8 amount,
                                          bool* failure) {
  return zx_shr_<zx_u32>(a, amount, failure);
}
constexpr static inline zx_u64 zx_shr_u64(zx_u64 a,
                                          zx_u8 amount,
                                          bool* failure) {
  return zx_shr_<zx_u64>(a, amount, failure);
}

template <typename T>
constexpr static inline T zx_shr_unsafe_(T a, zx_u8 amount) {
  constexpr zx_u8 N_BITS = sizeof(T) * 8;
  if (amount >= N_BITS) [[unlikely]] {
#ifdef ZX_DEBUG
    __builtin_trap();
#else
    return 0;
#endif
  }
  return a >> amount;
}
constexpr static inline zx_u8 zx_shr_unsafe_u8(zx_u8 a, zx_u8 amount) {
  return zx_shr_unsafe_<zx_u8>(a, amount);
}
constexpr static inline zx_u16 zx_shr_unsafe_u16(zx_u16 a, zx_u8 amount) {
  return zx_shr_unsafe_<zx_u16>(a, amount);
}
constexpr static inline zx_u32 zx_shr_unsafe_u32(zx_u32 a, zx_u8 amount) {
  return zx_shr_unsafe_<zx_u32>(a, amount);
}
constexpr static inline zx_u64 zx_shr_unsafe_u64(zx_u64 a, zx_u8 amount) {
  return zx_shr_unsafe_<zx_u64>(a, amount);
}

template <typename T>
constexpr static inline T zx_next_pow_two_(T a, bool* failure) {
  if (a == 0) {
    return T{1};
  }
  zx_u8 const shift = zx_bit_width_<T>(a - T{1});
  return zx_shl_<T>(T{1}, shift, failure);
}
constexpr static inline zx_u8 zx_next_pow_two_u8(zx_u8 a, bool* failure) {
  return zx_next_pow_two_<zx_u8>(a, failure);
}
constexpr static inline zx_u16 zx_next_pow_two_u16(zx_u16 a, bool* failure) {
  return zx_next_pow_two_<zx_u16>(a, failure);
}
constexpr static inline zx_u32 zx_next_pow_two_u32(zx_u32 a, bool* failure) {
  return zx_next_pow_two_<zx_u32>(a, failure);
}
constexpr static inline zx_u64 zx_next_pow_two_u64(zx_u64 a, bool* failure) {
  return zx_next_pow_two_<zx_u64>(a, failure);
}

template <typename T>
constexpr static inline T zx_next_pow_two_unsafe_(T a) {
  if (a == 0) {
    return T{1};
  }
  zx_u8 const shift = zx_bit_width_<T>(a - T{1});
  return zx_shl_unsafe_(T{1}, shift);
}
constexpr static inline zx_u8 zx_next_pow_two_unsafe_u8(zx_u8 a) {
  return zx_next_pow_two_unsafe_<zx_u8>(a);
}
constexpr static inline zx_u16 zx_next_pow_two_unsafe_u16(zx_u16 a) {
  return zx_next_pow_two_unsafe_<zx_u16>(a);
}
constexpr static inline zx_u32 zx_next_pow_two_unsafe_u32(zx_u32 a) {
  return zx_next_pow_two_unsafe_<zx_u32>(a);
}
constexpr static inline zx_u64 zx_next_pow_two_unsafe_u64(zx_u64 a) {
  return zx_next_pow_two_unsafe_<zx_u64>(a);
}

constexpr static inline bool zx_is_pow_two_u8(zx_u8 a) {
  return __builtin_popcountg(a) == 1;
}
constexpr static inline bool zx_is_pow_two_u16(zx_u16 a) {
  return __builtin_popcountg(a) == 1;
}
constexpr static inline bool zx_is_pow_two_u32(zx_u32 a) {
  return __builtin_popcountg(a) == 1;
}
constexpr static inline bool zx_is_pow_two_u64(zx_u64 a) {
  return __builtin_popcountg(a) == 1;
}

template <typename T>
constexpr static inline T zx_inverse_(T a) {
  return zx_wrapping_add_<T>(~a, T{1});
}
constexpr static inline zx_u8 zx_inverse_u8(zx_u8 a) {
  return zx_inverse_<zx_u8>(a);
}
constexpr static inline zx_u16 zx_inverse_u16(zx_u16 a) {
  return zx_inverse_<zx_u16>(a);
}
constexpr static inline zx_u32 zx_inverse_u32(zx_u32 a) {
  return zx_inverse_<zx_u32>(a);
}
constexpr static inline zx_u64 zx_inverse_u64(zx_u64 a) {
  return zx_inverse_<zx_u64>(a);
}

template <typename Tgt, typename Src>
constexpr static inline Tgt zx_int_cast_(Src a, bool* failure) {
  Tgt res;
  if (__builtin_add_overflow(a, Src{0}, &res)) [[unlikely]] {
    if (failure != nullptr) [[likely]] {
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
constexpr static inline zx_u8 zx_cast_to_u8(T a, bool* failure) {
  return zx_int_cast_<zx_u8, T>(a, failure);
}
template <typename T>
constexpr static inline zx_u16 zx_cast_to_u16(T a, bool* failure) {
  return zx_int_cast_<zx_u16, T>(a, failure);
}
template <typename T>
constexpr static inline zx_u32 zx_cast_to_u32(T a, bool* failure) {
  return zx_int_cast_<zx_u32, T>(a, failure);
}
template <typename T>
constexpr static inline zx_u64 zx_cast_to_u64(T a, bool* failure) {
  return zx_int_cast_<zx_u64, T>(a, failure);
}
template <typename T>
constexpr static inline zx_i8 zx_cast_to_i8(T a, bool* failure) {
  return zx_int_cast_<zx_i8, T>(a, failure);
}
template <typename T>
constexpr static inline zx_i16 zx_cast_to_i16(T a, bool* failure) {
  return zx_int_cast_<zx_i16, T>(a, failure);
}
template <typename T>
constexpr static inline zx_i32 zx_cast_to_i32(T a, bool* failure) {
  return zx_int_cast_<zx_i32, T>(a, failure);
}
template <typename T>
constexpr static inline zx_i64 zx_cast_to_i64(T a, bool* failure) {
  return zx_int_cast_<zx_i64, T>(a, failure);
}

template <typename Tgt, typename Src>
constexpr static inline Tgt zx_int_cast_unsafe_(Src a) {
  Tgt res;
  if (__builtin_add_overflow(a, Src{0}, &res)) [[unlikely]] {
    __builtin_trap();
  }
  return res;
}
template <typename T>
constexpr static inline zx_u8 zx_cast_unsafe_to_u8(T a, bool* failure) {
  return zx_int_cast_unsafe_<zx_u8, T>(a, failure);
}
template <typename T>
constexpr static inline zx_u16 zx_cast_unsafe_to_u16(T a, bool* failure) {
  return zx_int_cast_unsafe_<zx_u16, T>(a, failure);
}
template <typename T>
constexpr static inline zx_u32 zx_cast_unsafe_to_u32(T a, bool* failure) {
  return zx_int_cast_unsafe_<zx_u32, T>(a, failure);
}
template <typename T>
constexpr static inline zx_u64 zx_cast_unsafe_to_u64(T a, bool* failure) {
  return zx_int_cast_unsafe_<zx_u64, T>(a, failure);
}
template <typename T>
constexpr static inline zx_i8 zx_cast_unsafe_to_i8(T a, bool* failure) {
  return zx_int_cast_unsafe_<zx_i8, T>(a, failure);
}
template <typename T>
constexpr static inline zx_i16 zx_cast_unsafe_to_i16(T a, bool* failure) {
  return zx_int_cast_unsafe_<zx_i16, T>(a, failure);
}
template <typename T>
constexpr static inline zx_i32 zx_cast_unsafe_to_i32(T a, bool* failure) {
  return zx_int_cast_unsafe_<zx_i32, T>(a, failure);
}
template <typename T>
constexpr static inline zx_i64 zx_cast_unsafe_to_i64(T a, bool* failure) {
  return zx_int_cast_unsafe_<zx_i64, T>(a, failure);
}

#endif
