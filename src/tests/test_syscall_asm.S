.data
string:
	.ascii "Hello world!\n"
_string_end:

.text
.globl _start
_start:
	movl $4, %eax
	movl $1, %ebx
	movl $string, %ecx
	movl $(_string_end - string), %edx 
	int $0x80
	movl $1, %eax
	int $0x80

/* don't generate executable stack */
.section        .note.GNU-stack,"",@progbits

