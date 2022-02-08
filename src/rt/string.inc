.global string_alloc
.global string_concat
.align 2

.equ                string_pool_size, 64*1024
string_pool         .req x28
string_pool_pointer .req w27
