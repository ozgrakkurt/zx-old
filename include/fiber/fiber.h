#ifndef FIBER_H
#define FIBER_H

#include <shin/shin_def.h>

typedef struct FiberCtx {
  shin_u64 rsp;
  shin_u64 rbp;
  shin_u64 rip;
} FiberCtx;



#endif
