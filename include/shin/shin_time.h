#ifndef SHIN_TIME_H
#define SHIN_TIME_H

#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

#include <shin/shin_def.h>

typedef struct ShinTimer {
  shin_u64 start;
} ShinTimer;

static inline shin_u64 shin_timer_now_(void) {
  unsigned int aux;
  shin_u64 const ticker = __builtin_ia32_rdtscp(&aux);
  (void)aux;

  return ticker;
}

static inline ShinTimer ShinTimer_init(void) {
  shin_u64 const start = shin_timer_now_();

  return (ShinTimer){
      .start = start,
  };
}

// returns the number of clock cycles elapsed
static inline shin_u64 ShinTimer_lap(ShinTimer *timer) {
  shin_u64 const now = shin_timer_now_();
  shin_u64 const diff = now - timer->start;
  timer->start = now;
  return diff;
}

#endif
