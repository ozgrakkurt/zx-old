#ifndef FIBER_H
#define FIBER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

#ifndef __linux__
#error "only linux targets are supported"
#endif

#include <zx/zx_def.h>

typedef struct FiberCtx {
  zx_u64 rsp;
  zx_u64 rip;
} FiberCtx;
static_assert(sizeof(FiberCtx) == 16);

constexpr zx_u8 FIBER_STACK_RED_ZONE_SIZE = 128;
constexpr zx_u8 FIBER_STACK_ALIGN = 16;

typedef void fiber_startfn(FiberCtx const *parent_ctx, FiberCtx *fiber_ctx,
                           void *usr_ctx);

void fiber_start(FiberCtx *prev, FiberCtx *fiber_ctx, void *usr_ctx,
                 __attribute__((__noreturn__)) fiber_startfn startfn,
                 zx_u8 *fiber_stack);

void fiber_switch(FiberCtx *prev, FiberCtx const *next);

#ifdef __cplusplus
}
#endif

#endif
