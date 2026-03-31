#include <fiber/fiber.h>

__asm__(".type fiber_start, @function\n"
        ".global fiber_start\n"
        "fiber_start:\n\t"
        "pushq %rbx\n\t"
        "pushq %r12\n\t"
        "pushq %r13\n\t"
        "pushq %r14\n\t"
        "pushq %r15\n\t"
        "pushq %rbp\n\t"

        // save instruction pointer to label '0' so it jumps back to the ret
        // instruction
        "leaq 0f(%rip), %rax\n\t"
        // save current state to prev
        "movq %rsp, 0(%rdi)\n\t"
        "movq %rax, 8(%rdi)\n\t"

        // jump to startfn. function arguments (rdi, rsi and rdx) are already
        // set correctly. we only set the stack pointer
        "movq %r8, %rsp\n\t"
        "callq *%rcx\n\t"

        "0:\n\t"
        "popq %rbp\n\t"
        "popq %r15\n\t"
        "popq %r14\n\t"
        "popq %r13\n\t"
        "popq %r12\n\t"
        "popq %rbx\n\t"
        "ret\n\t");

__asm__(".type fiber_switch, @function\n"
        ".global fiber_switch\n"
        "fiber_switch:\n\t"
        "pushq %rbx\n\t"
        "pushq %r12\n\t"
        "pushq %r13\n\t"
        "pushq %r14\n\t"
        "pushq %r15\n\t"
        "pushq %rbp\n\t"

        // save instruction pointer to label '0' so it jumps back to the ret
        // instruction
        "leaq 0f(%rip), %rax\n\t"
        // save current state to prev
        "movq %rsp, 0(%rdi)\n\t"
        "movq %rax, 8(%rdi)\n\t"

        // transition into next state
        "movq 0(%rsi), %rsp\n\t"
        // jump to fiber
        "jmpq *8(%rsi)\n\t"

        "0:"
        "popq %rbp\n\t"
        "popq %r15\n\t"
        "popq %r14\n\t"
        "popq %r13\n\t"
        "popq %r12\n\t"
        "popq %rbx\n\t"
        "ret\n\t");
