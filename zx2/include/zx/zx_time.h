#ifndef ZX_TIME_H
#define ZX_TIME_H

#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

#include <zx/zx_def.h>

struct ZXTimer {
  zx_u64 start;

  static inline zx_u64 now_() {
    unsigned int aux;
    zx_u64 const ticker = __builtin_ia32_rdtscp(&aux);
    (void)aux;

    return ticker;
  }

  static inline ZXTimer init() {
    zx_u64 const start = now_();

    return ZXTimer{
        .start = start,
    };
  }

  // returns the number of ticks (roughly clock cycles) elapsed
  __attribute__((always_inline)) inline zx_u64 lap() {
    zx_u64 const now = now_();
    zx_u64 const diff = now - this->start;
    this->start = now;
    return diff;
  }

  inline zx_u64 elapsed() const {
    zx_u64 const now = now_();
    zx_u64 const diff = now - this->start;
    return diff;
  }
};

#endif
