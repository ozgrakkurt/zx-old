#ifndef SHIN_TIME_H
#define SHIN_TIME_H

#include <shin/shin_err.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ShinTimer {
  shin_u64 nanos;
} ShinTimer;

static inline ShinTimer ShinTimer_now() {
  struct timespec now;
  SHIN_ASSERT((0 == clock_gettime(CLOCK_MONOTONIC, &now)));

  constexpr shin_u64 NANOS_PER_SEC = 1000000000;
  return (ShinTimer){
      .nanos = ((shin_u64)now.tv_sec) * NANOS_PER_SEC + ((shin_u64)now.tv_nsec),
  };
}

// returns elapsed time in nanoseconds
static inline shin_u64 ShinTimer_lap(ShinTimer *timer) {
  shin_u64 const start = timer->nanos;
  ShinTimer const now = ShinTimer_now();
  timer->nanos = now.nanos;
  return now.nanos - start;
}

#ifdef __cplusplus
}
#endif

#endif
