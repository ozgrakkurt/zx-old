#ifndef FIBER_H
#define FIBER_H

#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

#ifndef __linux__
#error "only linux targets are supported"
#endif

#include <shin/shin_def.h>

typedef struct FiberCtx {
  shin_u64 rbx;
  shin_u64 r12;
  shin_u64 r13;
  shin_u64 r14;
  shin_u64 r15;
  shin_u64 rsp;
  shin_u64 rbp;
  shin_u64 rip;
} FiberCtx;
_Static_assert(sizeof(FiberCtx) == 64);

constexpr shin_usize FIBER_STACK_RED_ZONE_SIZE = 128;
constexpr shin_usize FIBER_STACK_ALIGN = 16;

typedef void fiber_startfn(FiberCtx const *parent_ctx, FiberCtx *fiber_ctx,
                           void *usr_ctx);
void fiber_start(FiberCtx *prev, FiberCtx *fiber_ctx, void *usr_ctx,
                 __attribute__((__noreturn__)) fiber_startfn startfn,
                 void *fiber_stack);
void fiber_switch(FiberCtx *prev, FiberCtx const *next);

#endif
