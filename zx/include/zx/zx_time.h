#ifndef ZX_TIME_H
#define ZX_TIME_H

#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

#include <zx/zx_def.h>

typedef struct ZXTimer {
  zx_u64 start;
} ZXTimer;

static inline zx_u64 zx_timer_now_(void) {
  unsigned int aux;
  zx_u64 const ticker = __builtin_ia32_rdtscp(&aux);
  (void)aux;

  return ticker;
}

static inline ZXTimer ZXTimer_init(void) {
  zx_u64 const start = zx_timer_now_();

  return (ZXTimer){
      .start = start,
  };
}

// returns the number of ticks (roughly clock cycles) elapsed
static inline zx_u64 ZXTimer_lap(ZXTimer *timer) {
  zx_u64 const now = zx_timer_now_();
  zx_u64 const diff = now - timer->start;
  timer->start = now;
  return diff;
}

static inline zx_u64 ZXTimer_elapsed(ZXTimer const *timer) {
  zx_u64 const now = zx_timer_now_();
  zx_u64 const diff = now - timer->start;
  return diff;
}

#endif
