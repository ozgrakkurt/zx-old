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

#endif
