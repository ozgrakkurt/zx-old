#ifndef FIBER_H
#define FIBER_H

#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

#ifndef __linux__
#error "only linux targets are supported"
#endif

#include <shin/shin_def.h>
#include <shin/shin_err.h>

typedef struct FiberCtx {
  shin_u64 rsp;
  shin_u64 rbp;
  shin_u64 rip;
} FiberCtx;
_Static_assert(sizeof(FiberCtx) == 24);

typedef ShinStatus fiber_startfn(FiberCtx *prev, void *ctx,
                                 ShinErrStack *err_stack);

ShinStatus fiber_start(FiberCtx *prev, void *ctx, ShinErrStack *err_stack,
                       fiber_startfn startfn, void *fiber_stack);
ShinStatus fiber_switch(FiberCtx *prev, FiberCtx const *next);

#endif
