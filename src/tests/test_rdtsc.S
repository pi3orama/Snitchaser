.text
.globl _start
_start:
	rdtsc
	movl $1, %eax
	int $0x80

