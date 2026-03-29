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
// typedef zx_f32 zx_v128_f32 __attribute__((vector_size(16)));
// typedef zx_f64 zx_v128_f64 __attribute__((vector_size(16)));

// typedef zx_u8 zx_v256_u8 __attribute__((vector_size(32)));
// typedef zx_u16 zx_v256_u16 __attribute__((vector_size(32)));
// typedef zx_u32 zx_v256_u32 __attribute__((vector_size(32)));
// typedef zx_u64 zx_v256_u64 __attribute__((vector_size(32)));
// typedef zx_i8 zx_v256_i8 __attribute__((vector_size(32)));
// typedef zx_i16 zx_v256_i16 __attribute__((vector_size(32)));
// typedef zx_i32 zx_v256_i32 __attribute__((vector_size(32)));
// typedef zx_i64 zx_v256_i64 __attribute__((vector_size(32)));
// typedef zx_f32 zx_v256_f32 __attribute__((vector_size(32)));
// typedef zx_f64 zx_v256_f64 __attribute__((vector_size(32)));

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
zx_v128_loadu_u8(void const *p) {
  zx_v128_u8 v;
  __builtin_memcpy(&v, p, 16);
  return v;
}

__attribute__((always_inline)) static inline zx_v128_i8
zx_v128_loadu_i8(void const *p) {
  zx_v128_i8 v;
  __builtin_memcpy(&v, p, 16);
  return v;
}

__attribute__((always_inline)) static inline zx_v128_u16
zx_v128_loadu_u16(void const *p) {
  zx_v128_u16 v;
  __builtin_memcpy(&v, p, 16);
  return v;
}

__attribute__((always_inline)) static inline zx_v128_i16
zx_v128_loadu_i16(void const *p) {
  zx_v128_i16 v;
  __builtin_memcpy(&v, p, 16);
  return v;
}

__attribute__((always_inline)) static inline zx_v128_u32
zx_v128_loadu_u32(void const *p) {
  zx_v128_u32 v;
  __builtin_memcpy(&v, p, 16);
  return v;
}

__attribute__((always_inline)) static inline zx_v128_i32
zx_v128_loadu_i32(void const *p) {
  zx_v128_i32 v;
  __builtin_memcpy(&v, p, 16);
  return v;
}

__attribute__((always_inline)) static inline zx_v128_u64
zx_v128_loadu_u64(void const *p) {
  zx_v128_u64 v;
  __builtin_memcpy(&v, p, 16);
  return v;
}

__attribute__((always_inline)) static inline zx_v128_i64
zx_v128_loadu_i64(void const *p) {
  zx_v128_i64 v;
  __builtin_memcpy(&v, p, 16);
  return v;
}

__attribute__((always_inline)) static inline void
zx_v128_storeu_u8(void *p, zx_v128_u8 v) {
  __builtin_memcpy(p, &v, 16);
}

__attribute__((always_inline)) static inline void
zx_v128_storeu_i8(void *p, zx_v128_i8 v) {
  __builtin_memcpy(p, &v, 16);
}

__attribute__((always_inline)) static inline void
zx_v128_storeu_u16(void *p, zx_v128_u16 v) {
  __builtin_memcpy(p, &v, 16);
}

__attribute__((always_inline)) static inline void
zx_v128_storeu_i16(void *p, zx_v128_i16 v) {
  __builtin_memcpy(p, &v, 16);
}

__attribute__((always_inline)) static inline void
zx_v128_storeu_u32(void *p, zx_v128_u32 v) {
  __builtin_memcpy(p, &v, 16);
}

__attribute__((always_inline)) static inline void
zx_v128_storeu_i32(void *p, zx_v128_i32 v) {
  __builtin_memcpy(p, &v, 16);
}

__attribute__((always_inline)) static inline void
zx_v128_storeu_u64(void *p, zx_v128_u64 v) {
  __builtin_memcpy(p, &v, 16);
}

__attribute__((always_inline)) static inline void
zx_v128_storeu_i64(void *p, zx_v128_i64 v) {
  __builtin_memcpy(p, &v, 16);
}

__attribute__((always_inline)) static inline zx_v128_u8
zx_v128_broadcast_u8(zx_u8 v) {
  return (zx_v128_u8){v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v};
}

__attribute__((always_inline)) static inline zx_v128_i8
zx_v128_broadcast_i8(zx_i8 v) {
  return (zx_v128_i8){v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v};
}

__attribute__((always_inline)) static inline zx_v128_u16
zx_v128_broadcast_u16(zx_u16 v) {
  return (zx_v128_u16){v, v, v, v, v, v, v, v};
}

__attribute__((always_inline)) static inline zx_v128_i16
zx_v128_broadcast_i16(zx_i16 v) {
  return (zx_v128_i16){v, v, v, v, v, v, v, v};
}

__attribute__((always_inline)) static inline zx_v128_u32
zx_v128_broadcast_u32(zx_u32 v) {
  return (zx_v128_u32){v, v, v, v};
}

__attribute__((always_inline)) static inline zx_v128_i32
zx_v128_broadcast_i32(zx_i32 v) {
  return (zx_v128_i32){v, v, v, v};
}

__attribute__((always_inline)) static inline zx_v128_u64
zx_v128_broadcast_u64(zx_u64 v) {
  return (zx_v128_u64){v, v};
}

__attribute__((always_inline)) static inline zx_v128_i64
zx_v128_broadcast_i64(zx_i64 v) {
  return (zx_v128_i64){v, v};
}

__attribute__((always_inline)) static inline zx_v128_u8
zx_v128_cmpeq_u8(zx_v128_u8 a, zx_v128_u8 b) {
  return a == b;
}

__attribute__((always_inline)) static inline zx_v128_i8
zx_v128_cmpeq_i8(zx_v128_i8 a, zx_v128_i8 b) {
  return a == b;
}

__attribute__((always_inline)) static inline zx_v128_u16
zx_v128_cmpeq_u16(zx_v128_u16 a, zx_v128_u16 b) {
  return a == b;
}

__attribute__((always_inline)) static inline zx_v128_i16
zx_v128_cmpeq_i16(zx_v128_i16 a, zx_v128_i16 b) {
  return a == b;
}

__attribute__((always_inline)) static inline zx_v128_u32
zx_v128_cmpeq_u32(zx_v128_u32 a, zx_v128_u32 b) {
  return a == b;
}

__attribute__((always_inline)) static inline zx_v128_i32
zx_v128_cmpeq_i32(zx_v128_i32 a, zx_v128_i32 b) {
  return a == b;
}

__attribute__((always_inline)) static inline zx_v128_u64
zx_v128_cmpeq_u64(zx_v128_u64 a, zx_v128_u64 b) {
  return a == b;
}

__attribute__((always_inline)) static inline zx_v128_i64
zx_v128_cmpeq_i64(zx_v128_i64 a, zx_v128_i64 b) {
  return a == b;
}

__attribute__((always_inline)) static inline zx_v128_u8
zx_v128_min_u8(zx_v128_u8 a, zx_v128_u8 b) {
  return __builtin_elementwise_min(a, b);
}

__attribute__((always_inline)) static inline zx_v128_i8
zx_v128_min_i8(zx_v128_i8 a, zx_v128_i8 b) {
  return __builtin_elementwise_min(a, b);
}

__attribute__((always_inline)) static inline zx_v128_u16
zx_v128_min_u16(zx_v128_u16 a, zx_v128_u16 b) {
  return __builtin_elementwise_min(a, b);
}

__attribute__((always_inline)) static inline zx_v128_i16
zx_v128_min_i16(zx_v128_i16 a, zx_v128_i16 b) {
  return __builtin_elementwise_min(a, b);
}

__attribute__((always_inline)) static inline zx_v128_u32
zx_v128_min_u32(zx_v128_u32 a, zx_v128_u32 b) {
  return __builtin_elementwise_min(a, b);
}

__attribute__((always_inline)) static inline zx_v128_i32
zx_v128_min_i32(zx_v128_i32 a, zx_v128_i32 b) {
  return __builtin_elementwise_min(a, b);
}

__attribute__((always_inline)) static inline zx_v128_u64
zx_v128_min_u64(zx_v128_u64 a, zx_v128_u64 b) {
  return __builtin_elementwise_min(a, b);
}

__attribute__((always_inline)) static inline zx_v128_i64
zx_v128_min_i64(zx_v128_i64 a, zx_v128_i64 b) {
  return __builtin_elementwise_min(a, b);
}

__attribute__((always_inline)) static inline zx_v128_u8
zx_v128_saturating_add_u8(zx_v128_u8 a, zx_v128_u8 b) {
  return __builtin_elementwise_add_sat(a, b);
}

__attribute__((always_inline)) static inline zx_v128_i8
zx_v128_saturating_add_i8(zx_v128_i8 a, zx_v128_i8 b) {
  return __builtin_elementwise_add_sat(a, b);
}

__attribute__((always_inline)) static inline zx_v128_u16
zx_v128_saturating_add_u16(zx_v128_u16 a, zx_v128_u16 b) {
  return __builtin_elementwise_add_sat(a, b);
}

__attribute__((always_inline)) static inline zx_v128_i16
zx_v128_saturating_add_i16(zx_v128_i16 a, zx_v128_i16 b) {
  return __builtin_elementwise_add_sat(a, b);
}

__attribute__((always_inline)) static inline zx_v128_u8
zx_v128_shufb_u8(zx_v128_u8 a, zx_v128_u8 b) {
  return __builtin_ia32_pshufb128(a, b);
}

__attribute__((always_inline)) static inline zx_u16
zx_v128_movmsk_u8(zx_v128_u8 v) {
  return (zx_u16)__builtin_ia32_pmovmskb128(v);
}

__attribute__((always_inline)) static inline zx_v128_i8
zx_v128_packus_i16(zx_v128_i16 a, zx_v128_i16 b) {
  return __builtin_ia32_packuswb128(a, b);
}

__attribute__((always_inline)) static inline zx_v128_i16
zx_v128_packus_i32(zx_v128_i32 a, zx_v128_i32 b) {
  return __builtin_ia32_packusdw128(a, b);
}

__attribute__((always_inline)) static inline zx_v512_u8
zx_v512_mask_loadu_u8(zx_v512_u8 src, zx_u64 mask, void const *p) {
  typedef char vt_ __attribute__((vector_size(64)));
  return __builtin_ia32_loaddquqi512_mask((vt_ const *)p, src, mask);
}

__attribute__((always_inline)) static inline void
zx_v512_mask_storeu_u8(void const *p, zx_u64 mask, zx_v512_u8 v) {
  typedef char vt_ __attribute__((vector_size(64)));
  __builtin_ia32_storedquqi512_mask((vt_ *)p, v, mask);
}

__attribute__((always_inline)) static inline zx_v512_u8 zx_v512_setzero_u8() {
  return (zx_v512_u8)(zx_v512_u64){0, 0, 0, 0, 0, 0, 0, 0};
}

__attribute__((always_inline)) static inline zx_u64
zx_v512_cmpeq_mask_u8(zx_v512_u8 a, zx_v512_u8 b) {
  return __builtin_ia32_cmpb512_mask(a, b, 0, ~(zx_u64){0});
}

__attribute__((always_inline)) static inline void
zx_v512_mask_compress_storeu_u8(void *p, zx_u64 mask, zx_v512_u8 v) {
  typedef char vt_ __attribute__((vector_size(64)));
  __builtin_ia32_compressstoreqi512_mask((vt_ *)p, v, mask);
}

#endif
