#ifndef ZX_FLOAT_H
#define ZX_FLOAT_H

typedef _Float16 zx_f16;
static_assert(sizeof(zx_f16) == 2);
static_assert(alignof(zx_f16) == 2);

typedef float zx_f32;
static_assert(sizeof(zx_f32) == 4);
static_assert(alignof(zx_f32) == 4);

typedef double zx_f64;
static_assert(sizeof(zx_f64) == 8);
static_assert(alignof(zx_f64) == 8);

#endif
