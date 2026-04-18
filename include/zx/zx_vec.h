#ifndef ZX_VEC_H
#define ZX_VEC_H

#include <zx/zx_float.h>
#include <zx/zx_int.h>

typedef zx_u8 zx_v128_u8 __attribute__((vector_size(16)));
typedef zx_u16 zx_v128_u16 __attribute__((vector_size(16)));
typedef zx_u32 zx_v128_u32 __attribute__((vector_size(16)));
typedef zx_u64 zx_v128_u64 __attribute__((vector_size(16)));
typedef zx_i8 zx_v128_i8 __attribute__((vector_size(16)));
typedef zx_i16 zx_v128_i16 __attribute__((vector_size(16)));
typedef zx_i32 zx_v128_i32 __attribute__((vector_size(16)));
typedef zx_i64 zx_v128_i64 __attribute__((vector_size(16)));
typedef zx_f32 zx_v128_f32 __attribute__((vector_size(16)));
typedef zx_f64 zx_v128_f64 __attribute__((vector_size(16)));

typedef zx_u8 zx_v256_u8 __attribute__((vector_size(32)));
typedef zx_u16 zx_v256_u16 __attribute__((vector_size(32)));
typedef zx_u32 zx_v256_u32 __attribute__((vector_size(32)));
typedef zx_u64 zx_v256_u64 __attribute__((vector_size(32)));
typedef zx_i8 zx_v256_i8 __attribute__((vector_size(32)));
typedef zx_i16 zx_v256_i16 __attribute__((vector_size(32)));
typedef zx_i32 zx_v256_i32 __attribute__((vector_size(32)));
typedef zx_i64 zx_v256_i64 __attribute__((vector_size(32)));
typedef zx_f32 zx_v256_f32 __attribute__((vector_size(32)));
typedef zx_f64 zx_v256_f64 __attribute__((vector_size(32)));

typedef zx_u8 zx_v512_u8 __attribute__((vector_size(64)));
typedef zx_u16 zx_v512_u16 __attribute__((vector_size(64)));
typedef zx_u32 zx_v512_u32 __attribute__((vector_size(64)));
typedef zx_u64 zx_v512_u64 __attribute__((vector_size(64)));
typedef zx_i8 zx_v512_i8 __attribute__((vector_size(64)));
typedef zx_i16 zx_v512_i16 __attribute__((vector_size(64)));
typedef zx_i32 zx_v512_i32 __attribute__((vector_size(64)));
typedef zx_i64 zx_v512_i64 __attribute__((vector_size(64)));
typedef zx_f32 zx_v512_f32 __attribute__((vector_size(64)));
typedef zx_f64 zx_v512_f64 __attribute__((vector_size(64)));

#endif
