#ifndef ZX_INT_H
#define ZX_INT_H

#include <zx/zx_assert.h>
#include <zx/zx_def.h>

constexpr zx_u8 ZX_U8_MAX = ~((zx_u8){0wbu});
constexpr zx_u16 ZX_U16_MAX = ~((zx_u16){0wbu});
constexpr zx_u32 ZX_U32_MAX = ~((zx_u32){0wbu});
constexpr zx_u64 ZX_U64_MAX = ~((zx_u64){0wbu});
constexpr zx_usize ZX_USIZE_MAX = ~((zx_usize){0wbu});

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

static inline zx_usize zx_chk_add_usize(zx_usize a, zx_usize b) {
  typedef unsigned _BitInt(sizeof(zx_usize) * 8 * 2) wide;
  wide const ret = (wide){a} + (wide){b};
  ZX_ASSERT((ret <= ZX_USIZE_MAX));
  return (zx_usize)ret;
}

#define zx_chk_add_g(a, b)                                                     \
  _Generic(((a) + (b)),                                                        \
      zx_u8: zx_chk_add_u8,                                                    \
      zx_u16: zx_chk_add_u16,                                                  \
      zx_u32: zx_chk_add_u32,                                                  \
      zx_u64: zx_chk_add_u64,                                                  \
      zx_usize: zx_chk_add_usize)((a), (b))

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

static inline zx_usize zx_chk_sub_usize(zx_usize a, zx_usize b) {
  ZX_ASSERT((a >= b));
  return a - b;
}

#define zx_chk_sub_g(a, b)                                                     \
  _Generic(((a) + (b)),                                                        \
      zx_u8: zx_chk_sub_u8,                                                    \
      zx_u16: zx_chk_sub_u16,                                                  \
      zx_u32: zx_chk_sub_u32,                                                  \
      zx_u64: zx_chk_sub_u64,                                                  \
      zx_usize: zx_chk_sub_usize)((a), (b))

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

static inline zx_usize zx_chk_mul_usize(zx_usize a, zx_usize b) {
  typedef unsigned _BitInt(sizeof(zx_usize) * 8 * 2) wide;
  wide const ret = (wide){a} * (wide){b};
  ZX_ASSERT((ret <= ZX_USIZE_MAX));
  return (zx_usize)ret;
}

#define zx_chk_mul_g(a, b)                                                     \
  _Generic(((a) + (b)),                                                        \
      zx_u8: zx_chk_mul_u8,                                                    \
      zx_u16: zx_chk_mul_u16,                                                  \
      zx_u32: zx_chk_mul_u32,                                                  \
      zx_u64: zx_chk_mul_u64,                                                  \
      zx_usize: zx_chk_mul_usize)((a), (b))

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

static inline zx_usize zx_chk_div_usize(zx_usize a, zx_usize b) {
  ZX_ASSERT((b > 0));
  return a / b;
}

#define zx_chk_div_g(a, b)                                                     \
  _Generic(((a) + (b)),                                                        \
      zx_u8: zx_chk_div_u8,                                                    \
      zx_u16: zx_chk_div_u16,                                                  \
      zx_u32: zx_chk_div_u32,                                                  \
      zx_u64: zx_chk_div_u64,                                                  \
      zx_usize: zx_chk_div_usize)((a), (b))

static inline zx_u8 zx_next_multiple_of_u8(zx_u8 a, zx_u8 b) {
  ZX_ASSERT((b > 0));
  zx_u16 const wa = a;
  zx_u16 const wb = b;
  zx_u16 const ret = (wa + wb - (zx_u16){1}) / wb * wb;
  ZX_ASSERT((ret <= ZX_U8_MAX));
  return (zx_u8)ret;
}

static inline zx_u16 zx_next_multiple_of_u16(zx_u16 a, zx_u16 b) {
  ZX_ASSERT((b > 0));
  zx_u32 const wa = a;
  zx_u32 const wb = b;
  zx_u32 const ret = (wa + wb - (zx_u32){1}) / wb * wb;
  ZX_ASSERT((ret <= ZX_U16_MAX));
  return (zx_u16)ret;
}

static inline zx_u32 zx_next_multiple_of_u32(zx_u32 a, zx_u32 b) {
  ZX_ASSERT((b > 0));
  zx_u64 const wa = a;
  zx_u64 const wb = b;
  zx_u64 const ret = (wa + wb - (zx_u64){1}) / wb * wb;
  ZX_ASSERT((ret <= ZX_U32_MAX));
  return (zx_u32)ret;
}

static inline zx_u64 zx_next_multiple_of_u64(zx_u64 a, zx_u64 b) {
  ZX_ASSERT((b > 0));
  zx_u128 const wa = a;
  zx_u128 const wb = b;
  zx_u128 const ret = (wa + wb - (zx_u128){1}) / wb * wb;
  ZX_ASSERT((ret <= ZX_U64_MAX));
  return (zx_u64)ret;
}

static inline zx_usize zx_next_multiple_of_usize(zx_usize a, zx_usize b) {
  typedef unsigned _BitInt(sizeof(zx_usize) * 8 * 2) wide;
  ZX_ASSERT((b > 0));
  wide const wa = a;
  wide const wb = b;
  wide const ret = (wa + wb - (wide){1}) / wb * wb;
  ZX_ASSERT((ret <= ZX_USIZE_MAX));
  return (zx_usize)ret;
}

#define zx_next_multiple_of_g(a, b)                                            \
  _Generic(((a) + (b)),                                                        \
      zx_u8: zx_next_multiple_of_u8,                                           \
      zx_u16: zx_next_multiple_of_u16,                                         \
      zx_u32: zx_next_multiple_of_u32,                                         \
      zx_u64: zx_next_multiple_of_u64,                                         \
      zx_usize: zx_next_multiple_of_usize)((a), (b))

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
static inline zx_usize zx_int_min_usize(zx_usize a, zx_usize b) {
  return a > b ? b : a;
}

#define zx_int_min_g(a, b)                                                     \
  _Generic(((a) + (b)),                                                        \
      zx_u8: zx_int_min_u8,                                                    \
      zx_u16: zx_int_min_u16,                                                  \
      zx_u32: zx_int_min_u32,                                                  \
      zx_u64: zx_int_min_u64,                                                  \
      zx_usize: zx_int_min_usize)((a), (b))

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
static inline zx_usize zx_int_max_usize(zx_usize a, zx_usize b) {
  return a < b ? b : a;
}

#define zx_int_max_g(a, b)                                                     \
  _Generic(((a) + (b)),                                                        \
      zx_u8: zx_int_max_u8,                                                    \
      zx_u16: zx_int_max_u16,                                                  \
      zx_u32: zx_int_max_u32,                                                  \
      zx_u64: zx_int_max_u64,                                                  \
      zx_usize: zx_int_max_usize)((a), (b))

// faster alternative for doing modulo on random numbers.
// basically same as doing `rand() % b` if rand is uniformly random.
static inline zx_u32 zx_fastmod_u32(zx_u32 a, zx_u32 b) {
  zx_u64 const mul = (zx_u64){a} * (zx_u64){b};
  return (zx_u32)(mul >> 32);
}

// faster alternative for doing modulo on random numbers.
// basically same as doing `rand() % b` if rand is uniformly random.
static inline zx_u64 zx_fastmod_u64(zx_u64 a, zx_u64 b) {
  zx_u128 const mul = (zx_u128){a} * (zx_u128){b};
  return (zx_u64)(mul >> 64);
}

// faster alternative for doing modulo on random numbers.
// basically same as doing `rand() % b` if rand is uniformly random.
static inline zx_usize zx_fastmod_usize(zx_usize a, zx_usize b) {
  constexpr zx_usize BIT_SIZE = sizeof(zx_usize) * 8;
  typedef unsigned _BitInt(BIT_SIZE * 2) wide;
  wide const mul = (wide){a} * (wide){b};
  return (zx_usize)(mul >> BIT_SIZE);
}

static inline zx_u8 zx_ctz_u8(zx_u8 i) { return __builtin_ctzg(i, 8); }

static inline zx_u8 zx_ctz_u16(zx_u16 i) { return __builtin_ctzg(i, 16); }

static inline zx_u8 zx_ctz_u32(zx_u32 i) { return __builtin_ctzg(i, 32); }

static inline zx_u8 zx_ctz_u64(zx_u64 i) { return __builtin_ctzg(i, 64); }

#define zx_ctz_g(i)                                                            \
  _Generic((i),                                                                \
      zx_u8: zx_ctz_u8,                                                        \
      zx_u16: zx_ctz_u16,                                                      \
      zx_u32: zx_ctz_u32,                                                      \
      zx_u64: zx_ctz_u64)((i))

static inline zx_u8 zx_clz_u8(zx_u8 i) { return __builtin_clzg(i, 8); }

static inline zx_u8 zx_clz_u16(zx_u16 i) { return __builtin_clzg(i, 16); }

static inline zx_u8 zx_clz_u32(zx_u32 i) { return __builtin_clzg(i, 32); }

static inline zx_u8 zx_clz_u64(zx_u64 i) { return __builtin_clzg(i, 64); }

#define zx_clz_g(i)                                                            \
  _Generic((i),                                                                \
      zx_u8: zx_clz_u8,                                                        \
      zx_u16: zx_clz_u16,                                                      \
      zx_u32: zx_clz_u32,                                                      \
      zx_u64: zx_clz_u64)((i))

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

#define zx_bit_width_g(i)                                                      \
  _Generic((i),                                                                \
      zx_u8: zx_bit_width_u8,                                                  \
      zx_u16: zx_bit_width_u16,                                                \
      zx_u32: zx_bit_width_u32,                                                \
      zx_u64: zx_bit_width_u64)((i))

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

#define zx_chk_shl_g(a, b)                                                     \
  _Generic((a),                                                                \
      zx_u8: zx_chk_shl_u8,                                                    \
      zx_u16: zx_chk_shl_u16,                                                  \
      zx_u32: zx_chk_shl_u32,                                                  \
      zx_u64: zx_chk_shl_u64)((a), (b))

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

#define zx_chk_shr_g(a, b)                                                     \
  _Generic((a),                                                                \
      zx_u8: zx_chk_shr_u8,                                                    \
      zx_u16: zx_chk_shr_u16,                                                  \
      zx_u32: zx_chk_shr_u32,                                                  \
      zx_u64: zx_chk_shr_u64)((a), (b))

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

#define zx_next_pow_two_g(i)                                                   \
  _Generic((i),                                                                \
      zx_u8: zx_next_pow_two_u8,                                               \
      zx_u16: zx_next_pow_two_u16,                                             \
      zx_u32: zx_next_pow_two_u32,                                             \
      zx_u64: zx_next_pow_two_u64)((i))

static inline bool zx_is_pow_two_u8(zx_u8 i) {
  return __builtin_popcountg(i) == 0;
}
static inline bool zx_is_pow_two_u16(zx_u16 i) {
  return __builtin_popcountg(i) == 0;
}
static inline bool zx_is_pow_two_u32(zx_u32 i) {
  return __builtin_popcountg(i) == 0;
}
static inline bool zx_is_pow_two_u64(zx_u64 i) {
  return __builtin_popcountg(i) == 0;
}
static inline bool zx_is_pow_two_usize(zx_usize i) {
  return __builtin_popcountg(i) == 0;
}

#define zx_is_pow_two_g(i)                                                     \
  _Generic((i),                                                                \
      zx_u8: zx_is_pow_two_u8,                                                 \
      zx_u16: zx_is_pow_two_u16,                                               \
      zx_u32: zx_is_pow_two_u32,                                               \
      zx_u64: zx_is_pow_two_u64,                                               \
      zx_usize: zx_is_pow_two_usize)((i))

#endif
