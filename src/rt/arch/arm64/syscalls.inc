.global close
.global exit
.global fsize
.global open
.global read
.global stat
.global write

.equ syscall_syscall,   0x0000
.equ syscall_exit,      0x0001
.equ syscall_fork,      0x0002
.equ syscall_read,      0x0003
.equ syscall_write,     0x0004
.equ syscall_open,      0x0005
.equ syscall_close,     0x0006
.equ syscall_wait4,     0x0007
.equ syscall_stat,      0x0153

.align 4
