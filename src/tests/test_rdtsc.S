.text
.globl _start
_start:
	rdtsc

	nop
	nop
	nop
	nop

	rdtsc

	nop
	nop
	nop
	nop

	jmp 1f
	nop
	nop
	nop
	1:
	nop
	rdtsc

	movl $1, %eax
	int $0x80

