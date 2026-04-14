#ifndef ZX_INT_H
#define ZX_INT_H

typedef unsigned char zx_u8_legacy;
static_assert(sizeof(zx_u8_legacy) == 1);
static_assert(alignof(zx_u8_legacy) == 1);

typedef unsigned short zx_u16_legacy;
static_assert(sizeof(zx_u16_legacy) == 2);
static_assert(alignof(zx_u16_legacy) == 2);

typedef unsigned int zx_u32_legacy;
static_assert(sizeof(zx_u32_legacy) == 4);
static_assert(alignof(zx_u32_legacy) == 4);

typedef unsigned long zx_u64_legacy;
static_assert(sizeof(zx_u64_legacy) == 8);
static_assert(alignof(zx_u64_legacy) == 8);

typedef char zx_i8_legacy;
static_assert(sizeof(zx_i8_legacy) == 1);
static_assert(alignof(zx_i8_legacy) == 1);

typedef short zx_i16_legacy;
static_assert(sizeof(zx_i16_legacy) == 2);
static_assert(alignof(zx_i16_legacy) == 2);

typedef int zx_i32_legacy;
static_assert(sizeof(zx_i32_legacy) == 4);
static_assert(alignof(zx_i32_legacy) == 4);

typedef long zx_i64_legacy;
static_assert(sizeof(zx_i64_legacy) == 8);
static_assert(alignof(zx_i64_legacy) == 8);

typedef unsigned _BitInt(8) zx_u8_raw;
typedef unsigned _BitInt(16) zx_u16_raw;
typedef unsigned _BitInt(32) zx_u32_raw;
typedef unsigned _BitInt(64) zx_u64_raw;
typedef unsigned _BitInt(128) zx_u128_raw;
typedef unsigned _BitInt(256) zx_u256_raw;
typedef _BitInt(8) zx_i8_raw;
typedef _BitInt(16) zx_i16_raw;
typedef _BitInt(32) zx_i32_raw;
typedef _BitInt(64) zx_i64_raw;
typedef _BitInt(128) zx_i128_raw;
typedef _BitInt(256) zx_i256_raw;

constexpr zx_u8_raw ZX_U8_RAW_MAX = ~(zx_u8_raw){0};
constexpr zx_u16_raw ZX_U16_RAW_MAX = ~(zx_u16_raw){0};
constexpr zx_u32_raw ZX_U32_RAW_MAX = ~(zx_u32_raw){0};
constexpr zx_u64_raw ZX_U64_RAW_MAX = ~(zx_u64_raw){0};
constexpr zx_u128_raw ZX_U128_RAW_MAX = ~(zx_u128_raw){0};
constexpr zx_u256_raw ZX_U256_RAW_MAX = ~(zx_u256_raw){0};

constexpr zx_i8_raw ZX_I8_RAW_MAX = (zx_i8_raw)(ZX_U8_RAW_MAX >> 1);
constexpr zx_i16_raw ZX_I16_RAW_MAX = (zx_i16_raw)(ZX_U16_RAW_MAX >> 1);
constexpr zx_i32_raw ZX_I32_RAW_MAX = (zx_i32_raw)(ZX_U32_RAW_MAX >> 1);
constexpr zx_i64_raw ZX_I64_RAW_MAX = (zx_i64_raw)(ZX_U64_RAW_MAX >> 1);
constexpr zx_i128_raw ZX_I128_RAW_MAX = (zx_i128_raw)(ZX_U128_RAW_MAX >> 1);
constexpr zx_i256_raw ZX_I256_RAW_MAX = (zx_i256_raw)(ZX_U256_RAW_MAX >> 1);
constexpr zx_i8_raw ZX_I8_RAW_MIN = ~ZX_I8_RAW_MAX;
constexpr zx_i16_raw ZX_I16_RAW_MIN = ~ZX_I16_RAW_MAX;
constexpr zx_i32_raw ZX_I32_RAW_MIN = ~ZX_I32_RAW_MAX;
constexpr zx_i64_raw ZX_I64_RAW_MIN = ~ZX_I64_RAW_MAX;
constexpr zx_i128_raw ZX_I128_RAW_MIN = ~ZX_I128_RAW_MAX;
constexpr zx_i256_raw ZX_I256_RAW_MIN = ~ZX_I256_RAW_MAX;

template <typename raw_t> class zx_int_unsigned_ {
private:
  using self_t = zx_int_unsigned_<raw_t>;

  raw_t m_val;

public:
  constexpr explicit zx_int_unsigned_(raw_t val) : m_val(val) {}

  __attribute__((always_inline)) constexpr inline self_t
  wrapping_add(self_t other) const {
    return self_t(this->m_val + other.m_val);
  }

  __attribute__((always_inline)) constexpr inline self_t
  saturating_add(self_t other) const {
    return self_t(__builtin_elementwise_add_sat(this->m_val, other.m_val));
  }

  __attribute__((always_inline)) constexpr inline self_t
  add(self_t other, bool *failure) const {
#ifdef ZX_DEV
    if (failure == nullptr) [[unlikely]] {
      __builtin_trap();
    }
#endif
    raw_t res;
    if (__builtin_add_overflow(this->m_val, other.m_val, &res)) [[unlikely]] {
      *failure = true;
    }
    return self_t(res);
  }

  __attribute__((always_inline)) constexpr inline self_t
  add_unsafe(self_t other) const {
#ifdef ZX_DEV
    raw_t res;
    if (__builtin_add_overflow(this->m_val, other.m_val, &res)) [[unlikely]] {
      __builtin_trap();
    }
    return res;
#else
    return self_t(this->m_val + other.m_val);
#endif
  }

  __attribute__((always_inline)) constexpr inline self_t
  wrapping_sub(self_t other) const {
    return self_t(this->m_val - other.m_val);
  }

  __attribute__((always_inline)) constexpr inline self_t
  saturating_sub(self_t other) const {
    return self_t(__builtin_elementwise_sub_sat(this->m_val, other.m_val));
  }

  __attribute__((always_inline)) constexpr inline self_t
  sub(self_t other, bool *failure) const {
#ifdef ZX_DEV
    if (failure == nullptr) [[unlikely]] {
      __builtin_trap();
    }
#endif
    raw_t res;
    if (__builtin_sub_overflow(this->m_val, other.m_val, &res)) [[unlikely]] {
      *failure = true;
    }
    return self_t(res);
  }

  __attribute__((always_inline)) constexpr inline self_t
  sub_unsafe(self_t other) const {
#ifdef ZX_DEV
    raw_t res;
    if (__builtin_sub_overflow(this->m_val, other.m_val, &res)) [[unlikely]] {
      __builtin_trap();
    }
    return res;
#else
    return self_t(this->m_val - other.m_val);
#endif
  }

  __attribute__((always_inline)) constexpr inline self_t
  wrapping_mul(self_t other) const {
    return self_t(this->m_val * other.m_val);
  }

  __attribute__((always_inline)) constexpr inline self_t
  saturating_mul(self_t other) const {
    raw_t res;
    if (__builtin_mul_overflow(this->m_val, other.m_val, &res)) [[unlikely]] {
      return self_t(~(raw_t){0});
    } else {
      return self_t(res);
    }
  }

  __attribute__((always_inline)) constexpr inline self_t
  mul(self_t other, bool *failure) const {
#ifdef ZX_DEV
    if (failure == nullptr) [[unlikely]] {
      __builtin_trap();
    }
#endif
    raw_t res;
    if (__builtin_mul_overflow(this->m_val, other.m_val, &res)) [[unlikely]] {
      *failure = true;
    }
    return self_t(res);
  }

  __attribute__((always_inline)) constexpr inline self_t
  mul_unsafe(self_t other) const {
#ifdef ZX_DEV
    raw_t res;
    if (__builtin_mul_overflow(this->m_val, other.m_val, &res)) [[unlikely]] {
      __builtin_trap();
    }
    return res;
#else
    return self_t(this->m_val * other.m_val);
#endif
  }

  __attribute__((always_inline)) constexpr inline self_t
  div(self_t other, bool *failure) const {
    if (other.m_val == 0) [[unlikely]] {
      *failure = true;
      return 0;
    }
    return self_t(this->m_val / other.m_val);
  }

  __attribute__((always_inline)) constexpr inline self_t
  div_unsafe(self_t other) const {
#ifdef ZX_DEV
    if (other.m_val == 0) [[unlikely]] {
      __builtin_trap();
    }
#else
    if (other.m_val == 0) [[unlikely]] {
      return self_t(0);
    }
#endif

    return self_t(this->m_val / other.m_val);
  }

  __attribute__((always_inline)) constexpr inline self_t
  mod(self_t other, bool *failure) const {
    if (other.m_val == 0) [[unlikely]] {
      *failure = true;
      return 0;
    }
    return self_t(this->m_val % other.m_val);
  }

  __attribute__((always_inline)) constexpr inline self_t
  mod_unsafe(self_t other) const {
#ifdef ZX_DEV
    if (other.m_val == 0) [[unlikely]] {
      __builtin_trap();
    }
#else
    if (other.m_val == 0) [[unlikely]] {
      return self_t(0);
    }
#endif

    return self_t(this->m_val % other.m_val);
  }

  __attribute__((always_inline)) constexpr inline self_t inverse() const {
    return self_t(~this->m_val + raw_t{1});
  }

  __attribute__((always_inline)) constexpr inline self_t operator~() const {
    return self_t(~this->m_val);
  }

  __attribute__((always_inline)) constexpr inline self_t
  operator&(self_t other) const {
    return self_t(this->m_val & other.m_val);
  }

  __attribute__((always_inline)) constexpr inline self_t
  operator|(self_t other) const {
    return self_t(this->m_val | other.m_val);
  }

  __attribute__((always_inline)) constexpr inline self_t
  operator^(self_t other) const {
    return self_t(this->m_val ^ other.m_val);
  }

  __attribute__((always_inline)) constexpr inline bool
  operator==(self_t other) const {
    return this->m_val == other.m_val;
  }

  __attribute__((always_inline)) constexpr inline bool
  operator!=(self_t other) const {
    return this->m_val != other.m_val;
  }

  __attribute__((always_inline)) constexpr inline bool
  operator>(self_t other) const {
    return this->m_val > other.m_val;
  }

  __attribute__((always_inline)) constexpr inline bool
  operator>=(self_t other) const {
    return this->m_val >= other.m_val;
  }

  __attribute__((always_inline)) constexpr inline bool
  operator<(self_t other) const {
    return this->m_val < other.m_val;
  }

  __attribute__((always_inline)) constexpr inline bool
  operator<=(self_t other) const {
    return this->m_val <= other.m_val;
  }

  // shr, shl.
};

using zx_u8 = zx_int_unsigned_<zx_u8_raw>;
constexpr zx_u8 ZX_U8_MAX = zx_u8(ZX_U8_RAW_MAX);
static_assert(sizeof(zx_u8) == sizeof(zx_u8_raw));
static_assert(alignof(zx_u8) == alignof(zx_u8_raw));

using zx_u16 = zx_int_unsigned_<zx_u16_raw>;
constexpr zx_u16 ZX_U16_MAX = zx_u16(ZX_U16_RAW_MAX);
static_assert(sizeof(zx_u16) == sizeof(zx_u16_raw));
static_assert(alignof(zx_u16) == alignof(zx_u16_raw));

using zx_u32 = zx_int_unsigned_<zx_u32_raw>;
constexpr zx_u32 ZX_U32_MAX = zx_u32(ZX_U32_RAW_MAX);
static_assert(sizeof(zx_u32) == sizeof(zx_u32_raw));
static_assert(alignof(zx_u32) == alignof(zx_u32_raw));

using zx_u64 = zx_int_unsigned_<zx_u64_raw>;
constexpr zx_u64 ZX_U64_MAX = zx_u64(ZX_U64_RAW_MAX);
static_assert(sizeof(zx_u64) == sizeof(zx_u64_raw));
static_assert(alignof(zx_u64) == alignof(zx_u64_raw));

using zx_u128 = zx_int_unsigned_<zx_u128_raw>;
constexpr zx_u128 ZX_U128_MAX = zx_u128(ZX_U128_RAW_MAX);
static_assert(sizeof(zx_u128) == sizeof(zx_u128_raw));
static_assert(alignof(zx_u128) == alignof(zx_u128_raw));

using zx_u256 = zx_int_unsigned_<zx_u256_raw>;
constexpr zx_u256 ZX_U256_MAX = zx_u256(ZX_U256_RAW_MAX);
static_assert(sizeof(zx_u256) == sizeof(zx_u256_raw));
static_assert(alignof(zx_u256) == alignof(zx_u256_raw));

constexpr zx_u8 ZX_U8_MAX = ~((zx_u8){0});
constexpr zx_u16 ZX_U16_MAX = ~((zx_u16){0});
constexpr zx_u32 ZX_U32_MAX = ~((zx_u32){0});
constexpr zx_u64 ZX_U64_MAX = ~((zx_u64){0});

static inline zx_u8 zx_chk_add_u8(zx_u8 a, zx_u8 b) {
  zx_u16 const ret = (zx_u16){a} + (zx_u16){b};
  ZX_ASSERT((ret <= ZX_U8_MAX));
  return (zx_u8)ret;
}

static inline zx_u16 zx_chk_add_u16(zx_u16 a, zx_u16 b) {
  zx_u32 const ret = (zx_u32){a} + (zx_u32){b};
  ZX_ASSERT((ret <= ZX_U16_MAX));
  return (zx_u16)ret;
}

static inline zx_u32 zx_chk_add_u32(zx_u32 a, zx_u32 b) {
  zx_u64 const ret = (zx_u64){a} + (zx_u64){b};
  ZX_ASSERT((ret <= ZX_U32_MAX));
  return (zx_u32)ret;
}

static inline zx_u64 zx_chk_add_u64(zx_u64 a, zx_u64 b) {
  zx_u128 const ret = (zx_u128){a} + (zx_u128){b};
  ZX_ASSERT((ret <= ZX_U64_MAX));
  return (zx_u64)ret;
}

static inline zx_u8 zx_chk_sub_u8(zx_u8 a, zx_u8 b) {
  ZX_ASSERT((a >= b));
  return a - b;
}

static inline zx_u16 zx_chk_sub_u16(zx_u16 a, zx_u16 b) {
  ZX_ASSERT((a >= b));
  return a - b;
}

static inline zx_u32 zx_chk_sub_u32(zx_u32 a, zx_u32 b) {
  ZX_ASSERT((a >= b));
  return a - b;
}

static inline zx_u64 zx_chk_sub_u64(zx_u64 a, zx_u64 b) {
  ZX_ASSERT((a >= b));
  return a - b;
}

static inline zx_u8 zx_chk_mul_u8(zx_u8 a, zx_u8 b) {
  zx_u16 const ret = (zx_u16){a} * (zx_u16){b};
  ZX_ASSERT((ret <= ZX_U8_MAX));
  return (zx_u8)ret;
}

static inline zx_u16 zx_chk_mul_u16(zx_u16 a, zx_u16 b) {
  zx_u32 const ret = (zx_u32){a} * (zx_u32){b};
  ZX_ASSERT((ret <= ZX_U16_MAX));
  return (zx_u16)ret;
}

static inline zx_u32 zx_chk_mul_u32(zx_u32 a, zx_u32 b) {
  zx_u64 const ret = (zx_u64){a} * (zx_u64){b};
  ZX_ASSERT((ret <= ZX_U32_MAX));
  return (zx_u32)ret;
}

static inline zx_u64 zx_chk_mul_u64(zx_u64 a, zx_u64 b) {
  zx_u128 const ret = (zx_u128){a} * (zx_u128){b};
  ZX_ASSERT((ret <= ZX_U64_MAX));
  return (zx_u64)ret;
}

static inline zx_u8 zx_chk_div_u8(zx_u8 a, zx_u8 b) {
  ZX_ASSERT((b > 0));
  return a / b;
}

static inline zx_u16 zx_chk_div_u16(zx_u16 a, zx_u16 b) {
  ZX_ASSERT((b > 0));
  return a / b;
}

static inline zx_u32 zx_chk_div_u32(zx_u32 a, zx_u32 b) {
  ZX_ASSERT((b > 0));
  return a / b;
}

static inline zx_u64 zx_chk_div_u64(zx_u64 a, zx_u64 b) {
  ZX_ASSERT((b > 0));
  return a / b;
}

static inline zx_u8 zx_next_multiple_of_u8(zx_u8 a, zx_u8 b) {
  ZX_ASSERT((b > 0));
  return (zx_chk_add_u8(a, b) - (zx_u8){1}) / b * b;
}

static inline zx_u16 zx_next_multiple_of_u16(zx_u16 a, zx_u16 b) {
  ZX_ASSERT((b > 0));
  return (zx_chk_add_u16(a, b) - (zx_u16){1}) / b * b;
}

static inline zx_u32 zx_next_multiple_of_u32(zx_u32 a, zx_u32 b) {
  ZX_ASSERT((b > 0));
  return (zx_chk_add_u32(a, b) - (zx_u32){1}) / b * b;
}

static inline zx_u64 zx_next_multiple_of_u64(zx_u64 a, zx_u64 b) {
  ZX_ASSERT((b > 0));
  return (zx_chk_add_u64(a, b) - (zx_u64){1}) / b * b;
}

static inline zx_u8 zx_int_min_u8(zx_u8 a, zx_u8 b) { return a > b ? b : a; }
static inline zx_u16 zx_int_min_u16(zx_u16 a, zx_u16 b) {
  return a > b ? b : a;
}
static inline zx_u32 zx_int_min_u32(zx_u32 a, zx_u32 b) {
  return a > b ? b : a;
}
static inline zx_u64 zx_int_min_u64(zx_u64 a, zx_u64 b) {
  return a > b ? b : a;
}

static inline zx_u8 zx_int_max_u8(zx_u8 a, zx_u8 b) { return a < b ? b : a; }
static inline zx_u16 zx_int_max_u16(zx_u16 a, zx_u16 b) {
  return a < b ? b : a;
}
static inline zx_u32 zx_int_max_u32(zx_u32 a, zx_u32 b) {
  return a < b ? b : a;
}
static inline zx_u64 zx_int_max_u64(zx_u64 a, zx_u64 b) {
  return a < b ? b : a;
}

// faster alternative for doing modulo on random numbers.
// basically same as doing `rand() % b` if rand is uniformly random.
static inline zx_u32 zx_rand_mod_u32(zx_u32 a, zx_u32 b) {
  zx_u64 const mul = (zx_u64){a} * (zx_u64){b};
  return (zx_u32)(mul >> 32);
}

// faster alternative for doing modulo on random numbers.
// basically same as doing `rand() % b` if rand is uniformly random.
static inline zx_u64 zx_rand_mod_u64(zx_u64 a, zx_u64 b) {
  zx_u128 const mul = (zx_u128){a} * (zx_u128){b};
  return (zx_u64)(mul >> 64);
}

static inline zx_u8 zx_ctz_u8(zx_u8 i) { return (zx_u8)__builtin_ctzg(i, 8); }

static inline zx_u8 zx_ctz_u16(zx_u16 i) {
  return (zx_u8)__builtin_ctzg(i, 16);
}

static inline zx_u8 zx_ctz_u32(zx_u32 i) {
  return (zx_u8)__builtin_ctzg(i, 32);
}

static inline zx_u8 zx_ctz_u64(zx_u64 i) {
  return (zx_u8)__builtin_ctzg(i, 64);
}

static inline zx_u8 zx_clz_u8(zx_u8 i) { return (zx_u8)__builtin_clzg(i, 8); }

static inline zx_u8 zx_clz_u16(zx_u16 i) {
  return (zx_u8)__builtin_clzg(i, 16);
}

static inline zx_u8 zx_clz_u32(zx_u32 i) {
  return (zx_u8)__builtin_clzg(i, 32);
}

static inline zx_u8 zx_clz_u64(zx_u64 i) {
  return (zx_u8)__builtin_clzg(i, 64);
}

static inline zx_u8 zx_bit_width_u8(zx_u8 i) {
  return (zx_u8){8} - zx_clz_u8(i);
}

static inline zx_u8 zx_bit_width_u16(zx_u16 i) {
  return (zx_u8){16} - zx_clz_u16(i);
}

static inline zx_u8 zx_bit_width_u32(zx_u32 i) {
  return (zx_u8){32} - zx_clz_u32(i);
}

static inline zx_u8 zx_bit_width_u64(zx_u64 i) {
  return (zx_u8){64} - zx_clz_u64(i);
}

static inline zx_u8 zx_chk_shl_u8(zx_u8 a, zx_u8 b) {
  ZX_ASSERT((b < 8));
  zx_u8 const max_shift = (zx_u8){8} - zx_bit_width_u8(a);
  ZX_ASSERT((b <= max_shift));
  return a << b;
}

static inline zx_u16 zx_chk_shl_u16(zx_u16 a, zx_u8 b) {
  ZX_ASSERT((b < 16));
  zx_u8 const max_shift = (zx_u8){16} - zx_bit_width_u16(a);
  ZX_ASSERT((b <= max_shift));
  return a << b;
}

static inline zx_u32 zx_chk_shl_u32(zx_u32 a, zx_u8 b) {
  ZX_ASSERT((b < 32));
  zx_u8 const max_shift = (zx_u8){32} - zx_bit_width_u32(a);
  ZX_ASSERT((b <= max_shift));
  return a << b;
}

static inline zx_u64 zx_chk_shl_u64(zx_u64 a, zx_u8 b) {
  ZX_ASSERT((b < 64));
  zx_u8 const max_shift = (zx_u8){64} - zx_bit_width_u64(a);
  ZX_ASSERT((b <= max_shift));
  return a << b;
}

static inline zx_u8 zx_chk_shr_u8(zx_u8 a, zx_u8 b) {
  ZX_ASSERT((b < 8));
  return a >> b;
}

static inline zx_u16 zx_chk_shr_u16(zx_u16 a, zx_u8 b) {
  ZX_ASSERT((b < 16));
  return a >> b;
}

static inline zx_u32 zx_chk_shr_u32(zx_u32 a, zx_u8 b) {
  ZX_ASSERT((b < 32));
  return a >> b;
}

static inline zx_u64 zx_chk_shr_u64(zx_u64 a, zx_u8 b) {
  ZX_ASSERT((b < 64));
  return a >> b;
}

static inline zx_u8 zx_next_pow_two_u8(zx_u8 i) {
  if (i == 0) {
    return 1;
  }
  zx_u8 const shift = zx_bit_width_u8(i - 1);
  return zx_chk_shl_u8(1, shift);
}

static inline zx_u16 zx_next_pow_two_u16(zx_u16 i) {
  if (i == 0) {
    return 1;
  }
  zx_u8 const shift = zx_bit_width_u16(i - 1);
  return zx_chk_shl_u16(1, shift);
}

static inline zx_u32 zx_next_pow_two_u32(zx_u32 i) {
  if (i == 0) {
    return 1;
  }
  zx_u8 const shift = zx_bit_width_u32(i - 1);
  return zx_chk_shl_u32(1, shift);
}

static inline zx_u64 zx_next_pow_two_u64(zx_u64 i) {
  if (i == 0) {
    return 1;
  }
  zx_u8 const shift = zx_bit_width_u64(i - 1);
  return zx_chk_shl_u64(1, shift);
}

static inline bool zx_is_pow_two_u8(zx_u8 i) {
  return __builtin_popcountg(i) == 1;
}
static inline bool zx_is_pow_two_u16(zx_u16 i) {
  return __builtin_popcountg(i) == 1;
}
static inline bool zx_is_pow_two_u32(zx_u32 i) {
  return __builtin_popcountg(i) == 1;
}
static inline bool zx_is_pow_two_u64(zx_u64 i) {
  return __builtin_popcountg(i) == 1;
}

static inline zx_u8 zx_popcnt_u8(zx_u8 i) {
  return (zx_u8)__builtin_popcountg(i);
}
static inline zx_u8 zx_popcnt_u16(zx_u16 i) {
  return (zx_u8)__builtin_popcountg(i);
}
static inline zx_u8 zx_popcnt_u32(zx_u32 i) {
  return (zx_u8)__builtin_popcountg(i);
}
static inline zx_u8 zx_popcnt_u64(zx_u64 i) {
  return (zx_u8)__builtin_popcountg(i);
}

#endif
