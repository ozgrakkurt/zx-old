#ifndef ZX_FLT_H
#define ZX_FLT_H

#include <zx/zx_def.h>

static inline zx_f16 zx_flt_max_f16(zx_f16 a, zx_f16 b) {
  return a < b ? b : a;
}

static inline zx_f32 zx_flt_max_f32(zx_f32 a, zx_f32 b) {
  return a < b ? b : a;
}

static inline zx_f64 zx_flt_max_f64(zx_f64 a, zx_f64 b) {
  return a < b ? b : a;
}

static inline zx_f16 zx_flt_min_f16(zx_f16 a, zx_f16 b) {
  return a > b ? b : a;
}

static inline zx_f32 zx_flt_min_f32(zx_f32 a, zx_f32 b) {
  return a > b ? b : a;
}

static inline zx_f64 zx_flt_min_f64(zx_f64 a, zx_f64 b) {
  return a > b ? b : a;
}

#endif
