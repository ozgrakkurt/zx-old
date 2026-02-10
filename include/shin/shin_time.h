#ifndef SHIN_TIME_H
#define SHIN_TIME_H

#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

typedef struct ShinTimer {
  shin_u64 ticker;
} ShinTimer;

static inline ShinTimer ShinTimer_now() {
  unsigned int aux;
  shin_u64 const ticker = __builtin_ia32_rdtscp(&aux);
  (void)aux;

  return (ShinTimer){
      .ticker = ticker,
  };
}

// returns elapsed time in nanoseconds
static inline shin_u64 ShinTimer_lap(ShinTimer *timer) {
  ShinTimer const now = ShinTimer_now();
  shin_u64 const diff = now.ticker - timer->ticker;
  *timer = now;

  return diff;
}

#endif
