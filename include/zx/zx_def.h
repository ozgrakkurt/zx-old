#ifndef ZX_DEF_H
#define ZX_DEF_H

typedef unsigned _BitInt(8) zx_u8;
typedef unsigned _BitInt(16) zx_u16;
typedef unsigned _BitInt(32) zx_u32;
typedef unsigned _BitInt(64) zx_u64;
typedef unsigned _BitInt(128) zx_u128;
typedef unsigned _BitInt(256) zx_u256;

typedef unsigned _BitInt(sizeof(void *) * 8) zx_usize;
static_assert(sizeof(zx_usize) == sizeof(void *));

typedef _BitInt(8) zx_i8;
typedef _BitInt(16) zx_i16;
typedef _BitInt(32) zx_i32;
typedef _BitInt(64) zx_i64;
typedef _BitInt(128) zx_i128;
typedef _BitInt(256) zx_i256;

typedef _BitInt(sizeof(void *) * 8) zx_isize;
static_assert(sizeof(zx_isize) == sizeof(void *));

typedef _Float16 zx_f16;
static_assert(sizeof(zx_f16) == 2);

typedef float zx_f32;
static_assert(sizeof(zx_f32) == 4);

typedef double zx_f64;
static_assert(sizeof(zx_f64) == 8);

#endif
