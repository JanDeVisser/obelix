.global close
.global exit
.global open
.global read
.global write

.equ syscall_syscall,   0x00
.equ syscall_exit,      0x01
.equ syscall_fork,      0x02
.equ syscall_read,      0x03
.equ syscall_write,     0x04
.equ syscall_open,      0x05
.equ syscall_close,     0x06
.equ syscall_wait4,     0x07

.align 4
