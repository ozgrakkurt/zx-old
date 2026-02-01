#ifndef SHIN_DEF_H
#define SHIN_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef(unsigned _BitInt(8)) shin_u8;
typedef(unsigned _BitInt(16)) shin_u16;
typedef(unsigned _BitInt(32)) shin_u32;
typedef(unsigned _BitInt(64)) shin_u64;
typedef(unsigned _BitInt(128)) shin_u128;
typedef(unsigned _BitInt(256)) shin_u256;

typedef _BitInt(8) shin_i8;
typedef _BitInt(16) shin_i16;
typedef _BitInt(32) shin_i32;
typedef _BitInt(64) shin_i64;
typedef _BitInt(128) shin_i128;
typedef _BitInt(256) shin_i256;

typedef _Float16 shin_f16;
typedef _Float32 shin_f32;
typedef _Float64 shin_f64;

#ifdef __cplusplus
}
#endif

#endif
