#ifndef ZX_DEF_H
#define ZX_DEF_H

namespace zx {

using u8_c = unsigned char;
static_assert(sizeof(u8_c) == 1);
static_assert(alignof(u8_c) == 1);

using u16_c = unsigned short;
static_assert(sizeof(u16_c) == 2);
static_assert(alignof(u16_c) == 2);

using u32_c = unsigned int;
static_assert(sizeof(u32_c) == 4);
static_assert(alignof(u32_c) == 4);

using u64_c = unsigned long;
static_assert(sizeof(u64_c) == 8);
static_assert(alignof(u64_c) == 8);

using i8_c = char;
static_assert(sizeof(i8_c) == 1);
static_assert(alignof(i8_c) == 1);

using i16_c = short;
static_assert(sizeof(i16_c) == 2);
static_assert(alignof(i16_c) == 2);

using i32_c = int;
static_assert(sizeof(i32_c) == 4);
static_assert(alignof(i32_c) == 4);

using i64_c = long;
static_assert(sizeof(i64_c) == 8);
static_assert(alignof(i64_c) == 8);

typedef unsigned _BitInt(8) u8;
typedef unsigned _BitInt(16) u16;
typedef unsigned _BitInt(32) u32;
typedef unsigned _BitInt(64) u64;
typedef unsigned _BitInt(128) u128;
typedef unsigned _BitInt(256) u256;
typedef _BitInt(8) i8;
typedef _BitInt(16) i16;
typedef _BitInt(32) i32;
typedef _BitInt(64) i64;
typedef _BitInt(128) i128;
typedef _BitInt(256) i256;

constexpr u8 U8_MAX = ~(u8){0};
constexpr u16 U16_MAX = ~(u16){0};
constexpr u32 U32_MAX = ~(u32){0};
constexpr u64 U64_MAX = ~(u64){0};
constexpr u128 U128_MAX = ~(u128){0};
constexpr u256 U256_MAX = ~(u256){0};

constexpr i8 I8_MAX = (i8)(U8_MAX >> 1);
constexpr i16 I16_MAX = (i16)(U16_MAX >> 1);
constexpr i32 I32_MAX = (i32)(U32_MAX >> 1);
constexpr i64 I64_MAX = (i64)(U64_MAX >> 1);
constexpr i128 I128_MAX = (i128)(U128_MAX >> 1);
constexpr i256 I256_MAX = (i256)(U256_MAX >> 1);
constexpr i8 I8_MIN = ~I8_MAX;
constexpr i16 I16_MIN = ~I16_MAX;
constexpr i32 I32_MIN = ~I32_MAX;
constexpr i64 I64_MIN = ~I64_MAX;
constexpr i128 I128_MIN = ~I128_MAX;
constexpr i256 I256_MIN = ~I256_MAX;

typedef _Float16 f16;
static_assert(sizeof(f16) == 2);
static_assert(alignof(f16) == 2);

typedef float f32;
static_assert(sizeof(f32) == 4);
static_assert(alignof(f32) == 4);

typedef double f64;
static_assert(sizeof(f64) == 8);
static_assert(alignof(f64) == 8);

} // namespace zx

#endif
