.text
.globl _start
_start:
	movl $0, %esi
begin:
	cmpl $2, %esi
	je end
	rdtsc
	nop
	rdtsc
	nop
	jmp 1f
	nop
	nop
	nop
	1:
	nop
	rdtsc
	inc %esi
	jmp begin
end:
	movl $1, %eax
	int $0x80

