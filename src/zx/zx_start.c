#ifndef __x86_64__
#error "only x86_64 is supported"
#endif

#ifndef __linux__
#error "only linux is supported"
#endif

__asm__(".type _start, @function\n"
        ".global _start\n"
        "_start:\n\t"
        "xorq %rbp, %rbp\n\t"
        "popq %rdi\n\t"
        "movq %rsp, %rsi\n\t"
        "andq $-16, %rsp\n\t"
        "callq main\n\t"
        "movq %rax, %rdi\n\t"
        "movq $60, %rax\n\t"
        "syscall\n\t");
