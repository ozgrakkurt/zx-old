#ifndef ZX_VEC_H
#define ZX_VEC_H

#ifndef __x86_64__
#error "only x86_64 is supported"
#endif

#include <zx/zx_def.h>

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

__attribute__((always_inline)) static inline zx_v128_u8
zx_v128_broadcast_u8(zx_u8 v) {
  return (zx_v128_u8){v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v};
}

__attribute__((always_inline)) static inline zx_v128_u8
zx_v128_cmpeq_u8(zx_v128_u8 a, zx_v128_u8 b) {
  return a == b;
}

__attribute__((always_inline)) static inline zx_v128_u8
zx_v128_shuf_u8(zx_v128_u8 a, zx_v128_u8 b) {
  return __builtin_ia32_pshufb128(a, b);
}

__attribute__((always_inline)) static inline zx_u16
zx_v128_movmsk_u8(zx_v128_u8 v) {
  return (zx_u16)__builtin_ia32_pmovmskb128(v);
}

__attribute__((always_inline)) static inline zx_v128_u8
zx_v128_loadu_u8(void const *p) {
  zx_v128_u8 v;
  __builtin_memcpy(&v, p, sizeof(v));
  return v;
}

__attribute__((always_inline)) static inline zx_v128_u8
zx_v128_load_u8(zx_v128_u8 const *p) {
  return *p;
}

__attribute__((always_inline)) static inline void
zx_v128_storeu_u8(void *p, zx_v128_u8 v) {
  __builtin_memcpy(p, &v, sizeof(v));
}

__attribute__((always_inline)) static inline void
zx_v128_store_u8(zx_v128_u8 *p, zx_v128_u8 v) {
  *p = v;
}

#endif
