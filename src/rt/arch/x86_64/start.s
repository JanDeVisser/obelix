.global _start

.align 2
_start:
    mov rax, 60
    mov rdi, 0
    syscall
