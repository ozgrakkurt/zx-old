.text:
.type _start, @function
.global _start
_start:
        xorq %rbp, %rbp
        popq %rdi
        movq %rsp, %rsi
        andq $-16, %rsp
        callq main
        movq %rax, %rdi
        movq $60, %rax
        syscall
