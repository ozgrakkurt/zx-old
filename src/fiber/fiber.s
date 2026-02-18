//void fiber_start(FiberCtx *prev, void* ctx, fiber_startfn startfn);
.type fiber_start, @function
.global fiber_start
fiber_start:
// save current state to prev
  movq %rbx, 0*8(%rdi)
  movq %r12, 1*8(%rdi)
  movq %r13, 2*8(%rdi)
  movq %r14, 3*8(%rdi)
  movq %r15, 4*8(%rdi)
  movq %rsp, 5*8(%rdi)
  movq %rbp, 6*8(%rdi)
// save instruction pointer to label '0' so it jumps back to the ret instruction
  leaq 0f(%rip), %rax
  movq %rax, 7*8(%rdi)

// call startfn. function arguments (rdi and rsi) are already set correctly.
  callq *%rdx
 
0:
  ret

// fill prev with current data and transition into next
//void fiber_switch(FiberCtx *prev, FiberCtx const *next);
.type fiber_switch, @function
.global fiber_switch
fiber_switch:
// save current state to prev
  movq %rbx, 0*8(%rdi)
  movq %r12, 1*8(%rdi)
  movq %r13, 2*8(%rdi)
  movq %r14, 3*8(%rdi)
  movq %r15, 4*8(%rdi)
  movq %rsp, 5*8(%rdi)
  movq %rbp, 6*8(%rdi)
// save instruction pointer to label '0' so it jumps back to the ret instruction
  leaq 0f(%rip), %rax
  movq %rax, 7*8(%rdi)

// transition into next state
  movq 0*8(%rsi), %rbx
  movq 1*8(%rsi), %r12
  movq 2*8(%rsi), %r13
  movq 3*8(%rsi), %r14
  movq 4*8(%rsi), %r15
  movq 5*8(%rsi), %rsp
  movq 6*8(%rsi), %rbp
// jump to save instruction pointer
  jmpq *7*8(%rsi)

0:
  ret

