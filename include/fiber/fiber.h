#ifndef FIBER_H
#define FIBER_H

#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

#ifndef __linux__
#error "only linux targets are supported"
#endif

#include <zx/zx_def.h>

extern "C" {

typedef struct FiberCtx {
  zx_u64 rsp;
  zx_u64 rip;
} FiberCtx;
static_assert(sizeof(FiberCtx) == 16);

typedef void fiber_startfn(FiberCtx const *parent_ctx, FiberCtx *fiber_ctx,
                           void *usr_ctx);

void fiber_start(FiberCtx *prev, FiberCtx *fiber_ctx, void *usr_ctx,
                 __attribute__((__noreturn__)) fiber_startfn startfn,
                 zx_u8 *fiber_stack);
}

constexpr zx_u8 FIBER_STACK_RED_ZONE_SIZE = 128;
constexpr zx_u8 FIBER_STACK_ALIGN = 16;

__attribute__((always_inline)) static inline void
fiber_switch(FiberCtx *prev, FiberCtx const *next) {
  __asm__ volatile(
      // save instruction pointer to label '0' so it jumps back to the ret
      // instruction
      "leaq 0f(%%rip), %%rax\n\t"
      // save current state to prev
      "movq %%rsp, 0(%[prev])\n\t"
      "movq %%rax, 8(%[prev])\n\t"
      // transition into next state
      "movq 0(%[next]), %%rsp\n\t"
      // jump to fiber
      "jmpq *8(%[next])\n\t"
      "0:"
      : [next] "+D"(next), [prev] "+S"(prev)
      :
      : "rbp", "rax", "rcx", "rdx", "rbx", "r8", "r9", "r10", "r11", "r12",
        "r13", "r14", "r15", "zmm0", "zmm1", "zmm2", "zmm3", "zmm4", "zmm5",
        "zmm6", "zmm7", "zmm8", "zmm9", "zmm10", "zmm11", "zmm12", "zmm13",
        "zmm14", "zmm15", "zmm16", "zmm17", "zmm18", "zmm19", "zmm20", "zmm21",
        "zmm22", "zmm23", "zmm24", "zmm25", "zmm26", "zmm27", "zmm28", "zmm29",
        "zmm30", "zmm31", "mm0", "mm1", "mm2", "mm3", "mm4", "mm5", "mm6",
        "mm7", "memory", "cc");
}

#endif
