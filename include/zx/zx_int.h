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

template <typename raw_type> class zx_int_unsigned_ {
private:
  static_assert(sizeof(raw_type) <= 8);
  static constexpr zx_u8_raw N_BITS = sizeof(raw_type) * 8;
  using zx_u8 = zx_int_unsigned_<zx_u8_raw>;
  using self_t = zx_int_unsigned_<raw_type>;

  raw_type m_val;

public:
  using raw_t = raw_type;

  constexpr explicit zx_int_unsigned_(raw_t val) : m_val(val) {}

  // Statically cast to target type. This will only compile if the target
  // type is able to hold any value of `this` type.
  template <typename tgt_t>
  __attribute__((always_inline)) constexpr inline tgt_t scast() const {
    return tgt_t(typename tgt_t::raw_t{this->m_val});
  }

  // Cast to target type with a runtime check.
  template <typename tgt_t>
  __attribute__((always_inline)) constexpr inline tgt_t
  rtcast(bool *failure) const {
    if (__builtin_add_overflow_p(this->m_val, raw_t(0), tgt_t::raw_t(0)))
        [[unlikely]] {
      *failure = true;
    }

    return tgt_t((typename tgt_t::raw_t)this->m_val);
  }

  // Cast to target type with a DEV mode only runtime check.
  template <typename tgt_t>
  __attribute__((always_inline)) constexpr inline tgt_t rtcast_unsafe() const {
#ifdef ZX_DEV
    if (__builtin_add_overflow_p(this->m_val, raw_t(0), tgt_t::raw_t(0)))
        [[unlikely]] {
      *failure = true;
    }
#endif

    return tgt_t((typename tgt_t::raw_t)this->m_val);
  }

  // Cast to target type using a c style cast. This works for truncation or
  // other lossy casts, but other cast methods should be preffered if possible.
  template <typename tgt_t>
  __attribute__((always_inline)) constexpr inline tgt_t unsafecast() const {
    return tgt_t((typename tgt_t::raw_t)this->m_val);
  }

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
    return self_t(res);
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
#ifdef ZX_DEV
    if (failure == nullptr) [[unlikely]] {
      __builtin_trap();
    }
#endif
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

  __attribute__((always_inline)) constexpr inline self_t
  rand_mod(self_t other) const {
    using wide_t = unsigned _BitInt(sizeof(raw_t) * 8 * 2);
    wide_t const mul = (wide_t){this->m_val} * (wide_t){other.m_val};
    return self_t((raw_t)(mul >> N_BITS));
  }

  __attribute__((always_inline)) constexpr inline self_t
  next_multiple_of(self_t other, bool *failure) const {
#ifdef ZX_DEV
    if (failure == nullptr) [[unlikely]] {
      __builtin_trap();
    }
#endif
    return this->add(self_t(1), failure)
        .div(other, failure)
        .mul(other, failure);
  }

  __attribute__((always_inline)) constexpr inline self_t
  next_multiple_of_unsafe(self_t other) const {
    return this->add_unsafe(self_t(1)).div_unsafe(other).mul_unsafe(other);
  }

  __attribute__((always_inline)) constexpr inline self_t
  min(self_t other) const {
    return (this->m_val < other.m_val) ? *this : other;
  }

  __attribute__((always_inline)) constexpr inline self_t
  max(self_t other) const {
    return (this->m_val > other.m_val) ? *this : other;
  }

  __attribute__((always_inline)) constexpr inline zx_u8 ctz() const {
    return zx_u8((zx_u8_raw)__builtin_ctzg(this->m_val, (int){N_BITS}));
  }

  __attribute__((always_inline)) constexpr inline zx_u8 clz() const {
    return zx_u8((zx_u8_raw)__builtin_clzg(this->m_val, (int){N_BITS}));
  }

  __attribute__((always_inline)) constexpr inline zx_u8 popcnt() const {
    return zx_u8((zx_u8_raw)__builtin_popcountg(this->m_val));
  }

  __attribute__((always_inline)) constexpr inline zx_u8 bit_width() const {
    return zx_u8(zx_u8(N_BITS).sub_unsafe(this->clz()));
  }

  __attribute__((always_inline)) constexpr inline self_t
  shl(zx_u8 shft, bool *failure) const {
#ifdef ZX_DEV
    if (failure == nullptr) [[unlikely]] {
      __builtin_trap();
    }
#endif
    if (shft >= zx_u8(N_BITS)) [[unlikely]] {
      *failure = true;
      return self_t(0);
    }
    return self_t(this->m_val << shft.scast<self_t>().m_val);
  }

  __attribute__((always_inline)) constexpr inline self_t
  shl_unsafe(zx_u8 shft) const {
#ifdef ZX_DEV
    if (shft >= zx_u8(N_BITS)) [[unlikely]] {
      __builtin_trap();
    }
#else
    if (shft >= zx_u8(N_BITS)) [[unlikely]] {
      return self_t(0);
    }
#endif
    return self_t(this->m_val << shft.scast<self_t>().m_val);
  }

  __attribute__((always_inline)) constexpr inline self_t
  shr(zx_u8 shft, bool *failure) const {
#ifdef ZX_DEV
    if (failure == nullptr) [[unlikely]] {
      __builtin_trap();
    }
#endif
    if (shft >= zx_u8(N_BITS)) [[unlikely]] {
      *failure = true;
      return self_t(0);
    }
    return self_t(this->m_val >> shft.scast<self_t>().m_val);
  }

  __attribute__((always_inline)) constexpr inline self_t
  shr_unsafe(zx_u8 shft) const {
#ifdef ZX_DEV
    if (shft >= zx_u8(N_BITS)) [[unlikely]] {
      __builtin_trap();
    }
#else
    if (shft >= zx_u8(N_BITS)) [[unlikely]] {
      return self_t(0);
    }
#endif
    return self_t(this->m_val >> shft.scast<self_t>().m_val);
  }

  __attribute__((always_inline)) constexpr inline self_t
  next_pow_two(bool *failure) const {
#ifdef ZX_DEV
    if (failure == nullptr) [[unlikely]] {
      __builtin_trap();
    }
#endif
    if (this->m_val == 0) {
      return self_t(1);
    }
    zx_u8 const shift = self_t(this->m_val - raw_t(1)).bit_width();
    return self_t(1).shl(shift, failure);
  }

  __attribute__((always_inline)) constexpr inline self_t
  next_pow_two_unsafe() const {
    if (this->m_val == 0) {
      return self_t(1);
    }
    zx_u8 const shift = self_t(this->m_val - raw_t(1)).bit_width();
    return self_t(1).shl_unsafe(shift);
  }

  __attribute__((always_inline)) constexpr inline bool is_pow_two() const {
    return this->popcnt() == zx_u8(1);
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
}; // class zx_int_unsigned_

template <typename raw_type> class zx_int_signed_ {
private:
  static_assert(sizeof(raw_type) <= 8);
  static constexpr zx_u8_raw N_BITS = sizeof(raw_type) * 8;
  using zx_u8 = zx_int_signed_<zx_u8_raw>;
  using self_t = zx_int_signed_<raw_type>;

  raw_type m_val;

public:
  using raw_t = raw_type;

  constexpr explicit zx_int_signed_(raw_t val) : m_val(val) {}

  // Statically cast to target type. This will only compile if the target
  // type is able to hold any value of `this` type.
  template <typename tgt_t>
  __attribute__((always_inline)) constexpr inline tgt_t scast() const {
    return tgt_t(typename tgt_t::raw_t{this->m_val});
  }

  // Cast to target type with a runtime check.
  template <typename tgt_t>
  __attribute__((always_inline)) constexpr inline tgt_t
  rtcast(bool *failure) const {
    if (__builtin_add_overflow_p(this->m_val, raw_t(0), tgt_t::raw_t(0)))
        [[unlikely]] {
      *failure = true;
    }

    return tgt_t((typename tgt_t::raw_t)this->m_val);
  }

  // Cast to target type with a DEV mode only runtime check.
  template <typename tgt_t>
  __attribute__((always_inline)) constexpr inline tgt_t rtcast_unsafe() const {
#ifdef ZX_DEV
    if (__builtin_add_overflow_p(this->m_val, raw_t(0), tgt_t::raw_t(0)))
        [[unlikely]] {
      *failure = true;
    }
#endif

    return tgt_t((typename tgt_t::raw_t)this->m_val);
  }

  // Cast to target type using a c style cast. This works for truncation or
  // other lossy casts, but other cast methods should be preffered if possible.
  template <typename tgt_t>
  __attribute__((always_inline)) constexpr inline tgt_t unsafecast() const {
    return tgt_t((typename tgt_t::raw_t)this->m_val);
  }

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
    return self_t(res);
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
  min(self_t other) const {
    return (this->m_val < other.m_val) ? *this : other;
  }

  __attribute__((always_inline)) constexpr inline self_t
  max(self_t other) const {
    return (this->m_val > other.m_val) ? *this : other;
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
}; // class zx_int_signed_

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

using zx_i8 = zx_int_signed_<zx_i8_raw>;
constexpr zx_i8 ZX_I8_MAX = zx_i8(ZX_I8_RAW_MAX);
constexpr zx_i8 ZX_I8_MIN = zx_i8(ZX_I8_RAW_MIN);
static_assert(sizeof(zx_i8) == sizeof(zx_i8_raw));
static_assert(alignof(zx_i8) == alignof(zx_i8_raw));

using zx_i16 = zx_int_signed_<zx_i16_raw>;
constexpr zx_i16 ZX_I16_MAX = zx_i16(ZX_I16_RAW_MAX);
constexpr zx_i16 ZX_I16_MIN = zx_i16(ZX_I16_RAW_MIN);
static_assert(sizeof(zx_i16) == sizeof(zx_i16_raw));
static_assert(alignof(zx_i16) == alignof(zx_i16_raw));

using zx_i32 = zx_int_signed_<zx_i32_raw>;
constexpr zx_i32 ZX_I32_MAX = zx_i32(ZX_I32_RAW_MAX);
constexpr zx_i32 ZX_I32_MIN = zx_i32(ZX_I32_RAW_MIN);
static_assert(sizeof(zx_i32) == sizeof(zx_i32_raw));
static_assert(alignof(zx_i32) == alignof(zx_i32_raw));

using zx_i64 = zx_int_signed_<zx_i64_raw>;
constexpr zx_i64 ZX_I64_MAX = zx_i64(ZX_I64_RAW_MAX);
constexpr zx_i64 ZX_I64_MIN = zx_i64(ZX_I64_RAW_MIN);
static_assert(sizeof(zx_i64) == sizeof(zx_i64_raw));
static_assert(alignof(zx_i64) == alignof(zx_i64_raw));

template <typename raw_type> class zx_int_big_ {
private:
  raw_type m_val;

public:
  using raw_t = raw_type;
  constexpr explicit zx_int_big_(raw_t val) : m_val(val) {}
};

using zx_u128 = zx_int_big_<zx_u128_raw>;
constexpr zx_u128 ZX_U128_MAX = zx_u128(ZX_U128_RAW_MAX);
static_assert(sizeof(zx_u128) == sizeof(zx_u128_raw));
static_assert(alignof(zx_u128) == alignof(zx_u128_raw));

using zx_u256 = zx_int_big_<zx_u256_raw>;
constexpr zx_u256 ZX_U256_MAX = zx_u256(ZX_U256_RAW_MAX);
static_assert(sizeof(zx_u256) == sizeof(zx_u256_raw));
static_assert(alignof(zx_u256) == alignof(zx_u256_raw));

using zx_i128 = zx_int_big_<zx_i128_raw>;
constexpr zx_i128 ZX_I128_MAX = zx_i128(ZX_I128_RAW_MAX);
constexpr zx_i128 ZX_I128_MIN = zx_i128(ZX_I128_RAW_MIN);
static_assert(sizeof(zx_i128) == sizeof(zx_i128_raw));
static_assert(alignof(zx_i128) == alignof(zx_i128_raw));

using zx_i256 = zx_int_big_<zx_i256_raw>;
constexpr zx_i256 ZX_I256_MAX = zx_i256(ZX_I256_RAW_MAX);
constexpr zx_i256 ZX_I256_MIN = zx_i256(ZX_I256_RAW_MIN);
static_assert(sizeof(zx_i256) == sizeof(zx_i256_raw));
static_assert(alignof(zx_i256) == alignof(zx_i256_raw));

#endif
