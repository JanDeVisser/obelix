//
// Assembler program to print "Hello World!"
// to stdout.
//
// X0-X2 - parameters to linux function services
// X16 - linux function number
//

// $ as -o hello.o hello.s
// $ ld -o hello hello.o -lSystem -syslibroot `xcrun -sdk macosx --show-sdk-path` -e _start -arch arm64


.global _start             // Provide program starting address to linker
.align 2

// Setup the parameters to print hello world
// and then call MacOS to do it.

_start:
	adr x0,pool_available
	ldr x1,[pool_available]
	cmp x1,string_pool_top


helloworld:      .ascii  "Hello World!\n"

.data
pool_available:
	.quad string_pool

string_pool:
	.space 2048,0
string_pool_top:
