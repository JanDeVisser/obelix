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
        mov fp,sp
        mov x0, #1         // 1 = StdOut
        adr x1, helloworld // string to print
        mov x2, #13        // length of our string
        bl sys$write
        bl sys$exit

helloworld:      .ascii  "Hello World!\n"
