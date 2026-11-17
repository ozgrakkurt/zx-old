.type fiber_start, @function
.global fiber_start
fiber_start:
  pushq %rbx
  pushq %r12
  pushq %r13
  pushq %r14
  pushq %r15
  pushq %rbp

// save instruction pointer to label '0' so it jumps back to the ret instruction
  leaq 0f(%rip), %rax
// save current state to prev
  movq %rsp, 0(%rdi)
  movq %rax, 8(%rdi)

// jump to startfn. function arguments (rdi, rsi and rdx) are already set correctly.
// we only set the stack pointer
  movq %r8, %rsp
  callq *%rcx

0:
  popq %rbp
  popq %r15
  popq %r14
  popq %r13
  popq %r12
  popq %rbx
  ret

.type fiber_switch, @function
.global fiber_switch
fiber_switch:
  pushq %rbx
  pushq %r12
  pushq %r13
  pushq %r14
  pushq %r15
  pushq %rbp

// save instruction pointer to label '0' so it jumps back to the ret instruction
  leaq 0f(%rip), %rax
// save current state to prev
  movq %rsp, 0(%rdi)
  movq %rax, 8(%rdi)

// transition into next state
  movq 0(%rsi), %rsp
// jump to fiber
  jmpq *8(%rsi)

0:
  popq %rbp
  popq %r15
  popq %r14
  popq %r13
  popq %r12
  popq %rbx
  ret

