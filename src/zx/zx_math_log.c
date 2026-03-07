#include <zx/zx_def.h>

#ifndef __FMA__
#error "fma is required"
#endif

#include "./math_log_data/zx_math_log_data.h"

// use __builtin_elementwise_fma instead of normal fma because it doesn't use
// errno https://github.com/llvm/llvm-project/pull/126288

#define T __log_data.tab
#define T2 __log_data.tab2
#define B __log_data.poly1
#define A __log_data.poly
#define Ln2hi __log_data.ln2hi
#define Ln2lo __log_data.ln2lo
#define N (1 << LOG_TABLE_BITS)
#define OFF 0x3fe6000000000000

// from other parts of musl-libc
#define INFINITY __builtin_inff()
#define WANT_ROUNDING 1

#define asuint(f)                                                              \
  ((union {                                                                    \
    float _f;                                                                  \
    zx_u32 _i;                                                                 \
  }){f})                                                                       \
      ._i
#define asfloat(i)                                                             \
  ((union {                                                                    \
    zx_u32 _i;                                                                 \
    float _f;                                                                  \
  }){i})                                                                       \
      ._f
#define asuint64(f)                                                            \
  ((union {                                                                    \
    double _f;                                                                 \
    zx_u64 _i;                                                                 \
  }){f})                                                                       \
      ._i
#define asdouble(i)                                                            \
  ((union {                                                                    \
    zx_u64 _i;                                                                 \
    double _f;                                                                 \
  }){i})                                                                       \
      ._f

static inline double eval_as_double(double x) {
  double y = x;
  return y;
}

// taken from log.c in musl-libc

/* Top 16 bits of a double.  */
static inline zx_u32 top16(double x) { return asuint64(x) >> 48; }

__attribute__((no_builtin)) double log(double x) {
  double w, z, r, r2, r3, y, invc, logc, kd, hi, lo;
  zx_u64 ix, iz, tmp;
  zx_u32 top;
  int k, i;

  ix = asuint64(x);
  top = top16(x);
#define LO asuint64(1.0 - 0x1p-4)
#define HI asuint64(1.0 + 0x1.09p-4)
  if (ix - LO < HI - LO) {
    /* Handle close to 1.0 inputs separately.  */
    /* Fix sign of zero with downward rounding when x==1.  */
    if (WANT_ROUNDING && ix == asuint64(1.0))
      return 0;
    r = x - 1.0;
    r2 = r * r;
    r3 = r * r2;
    y = r3 * (B[1] + r * B[2] + r2 * B[3] +
              r3 * (B[4] + r * B[5] + r2 * B[6] +
                    r3 * (B[7] + r * B[8] + r2 * B[9] + r3 * B[10])));
    /* Worst-case error is around 0.507 ULP.  */
    w = r * 0x1p27;
    double rhi = r + w - w;
    double rlo = r - rhi;
    w = rhi * rhi * B[0]; /* B[0] == -0.5.  */
    hi = r + w;
    lo = r - hi + w;
    lo += B[0] * rlo * (rhi + r);
    y += lo;
    y += hi;
    return eval_as_double(y);
  }
  if (top - 0x0010 >= 0x7ff0 - 0x0010) {
    /* x < 0x1p-1022 or inf or nan.  */
    if (ix * 2 == 0)
      // division by zero but we don't use errno
      return 1;
    if (ix == asuint64(INFINITY)) /* log(inf) == inf.  */
      return x;
    if ((top & 0x8000) || (top & 0x7ff0) == 0x7ff0)
      // invalid but we don't use errno
      return x;
    /* x is subnormal, normalize it.  */
    ix = asuint64(x * 0x1p52);
    ix -= 52ULL << 52;
  }

  /* x = 2^k z; where z is in range [OFF,2*OFF) and exact.
     The range is split into N subintervals.
     The ith subinterval contains z and c is near its center.  */
  tmp = ix - OFF;
  i = (tmp >> (52 - LOG_TABLE_BITS)) % N;
  k = (zx_i64)tmp >> 52; /* arithmetic shift */
  iz = ix - (tmp & 0xfffULL << 52);
  invc = T[i].invc;
  logc = T[i].logc;
  z = asdouble(iz);

  /* log(x) = log1p(z/c-1) + log(c) + k*Ln2.  */
  /* r ~= z/c - 1, |r| < 1/(2*N).  */
  /* rounding error: 0x1p-55/N.  */
  r = __builtin_elementwise_fma(z, invc, -1.0);
  kd = (double)k;

  /* hi + lo = r + log(c) + k*Ln2.  */
  w = kd * Ln2hi + logc;
  hi = w + r;
  lo = w - hi + r + kd * Ln2lo;

  /* log(x) = lo + (log1p(r) - r) + hi.  */
  r2 = r * r; /* rounding error: 0x1p-54/N^2.  */
  /* Worst case error if |y| > 0x1p-5:
     0.5 + 4.13/N + abs-poly-error*2^57 ULP (+ 0.002 ULP without fma)
     Worst case error if |y| > 0x1p-4:
     0.5 + 2.06/N + abs-poly-error*2^56 ULP (+ 0.001 ULP without fma).  */
  y = lo + r2 * A[0] + r * r2 * (A[1] + r * A[2] + r2 * (A[3] + r * A[4])) + hi;
  return eval_as_double(y);
}
