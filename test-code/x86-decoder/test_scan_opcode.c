#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


extern uint8_t *
next_inst(uint8_t * stream);

static void func2(void)
{
	asm volatile (
	"rcll $0x12, 0x12345678\n"
	"rcll $0x12, (%eax)\n"
	"rcll $0x12, %eax\n"
	"rclw $0x12, %ax\n"
	"rclb $0x12, %al\n"
	"roll $0x12, 0x12345678\n"
	"roll $0x12, (%eax)\n"
	"roll $0x12, %eax\n"
	"rolw $0x12, %ax\n"
	"rolb $0x12, %al\n"
	"lesl 0x1234(%eax, %edx, 4), %ecx\n"
	"ldsw 0x1234(%eax, %edx, 4), %cx\n"
	"movb $0x12, (%eax)\n"
	"movw $0x1234, (%eax)\n"
	"movl $0x12345678, (%ebx, %ecx, 1)\n"

	"rolb $1, (%eax)\n"
	"roll $1, (%eax)\n"
	"roll $1, %eax\n"
	"rolb %cl, %al\n"
	"rolb %cl, (%eax)\n"
	"rolw %cl, (%eax)\n"
	"roll %cl, (%eax)\n"
	"aam $0x23\n"
	"aad $0x23\n"
	"xlat\n"
	"xlatb\n"
	"inb $0x12, %al\n"
	"inw $0x43, %ax\n"
	"inl $0x43, %eax\n"
	"outb %al, $0x12\n"
	"outw %ax, $0x12\n"
	"outl %eax, $0x12\n"

	"lock movsl\n"
	"hlt\n"
	"cmc\n"
	"testb $0x12, 0x12345678(%ebx, %eax, 4)\n"
	"testw 0x1234, %ax\n"
	"test 0x1234, %eax\n"
	"notb %al\n"
	"notb (%eax)\n"
	"notl (%eax)\n"
	"negl %eax\n"
	"imulb %al\n"
	"imulb %ah\n"
	"imulw %ax\n"
	"imulw 0x1234(%eax)\n"
	"imull (%eax)\n"
	"imull (%ebx)\n"
	"divl (%ebx)\n"
	"divl %ebx\n"
			"\n"
			);
}

static void func(void)
{
	asm volatile (
			"addb %bl, %ch\n"
			"addb %ch, 0x12345678\n"
			"addb %bh, 0x12345(%ecx, %ebx, 2)\n"
			"addr16 addb %ch, 0x1234\n"
			"addr16 addb %ch, (%bx, %si)\n"
			"addr16 addb %ch, 0x1234(%bx, %si)\n"

			"addw %bx, %cx\n"
			"addw %bx, 0x12345678\n"
			"addw %bx, (%ecx)\n"
			"addw %bx, (%ecx, %eax, 4)\n"
			"addw %bx, 0x45671245(%ecx, %ebx, 2)\n"

			"addl %ebx, %ecx\n"
			"addl %ebx, 0x12345678\n"
			"addl %ebx, (%ecx)\n"
			"addl %ebx, (%ecx, %eax, 4)\n"
			"addl %ebx, 0x45671245(%ecx, %ebx, 2)\n"

			"addb 0x12345678, %bh\n"

			"addw 0x34567890, %bx\n"
			"addl 0x34567890, %ebx\n"
			"addl 0x34567890(%ebx), %ebx\n"

			"addb $0x12, %al\n"
			"addw $0x1234, %ax\n"
			"addl $0x1234, %eax\n"

			"push %es\n"
			"pop %es\n"

			"adcl 0x1234, %ebx\n"
			"push %ss\n"
			"pop %ss\n"

			"inc %eax\n"
			"inc %ebx\n"
			"inc %ecx\n"
			"inc %edx\n"
			"inc %esi\n"
			"inc %edi\n"
			"inc %ebp\n"
			"inc %esp\n"

			"pushw %ax\n"
			"pushl %ebx\n"
			"pushl %ecx\n"
			"pushl %edx\n"
			"pushl %esi\n"
			"pushl %ebp\n"
			"pushl %esp\n"

			"subb $0x13, 0x12346789\n"
			"andb $0x13, 0x12345678(%ebx, %eax, 4)\n"

			"subw $0x13, 0x12346789\n"
			"andw $0x13, 0x12345678(%ebx, %eax, 4)\n"

			"subl $0x13, 0x12346789\n"
			"andl $0x13, 0x12345678(%ebx, %eax, 4)\n"

			"addr16 subw $0x13, 0x1234\n"
			"addr16 andw $0x13, 0x1234(%bx)\n"

			"sbbb   $0x34,0x78563467(%ecx,%ecx,4)\n"
			"bound  %eax,0x12345678\n"
			"arpl   %ax,0x12345678(%ebx,%edx,2)\n"
			"addl    %eax,%fs:0x12345678\n"
			"addl    %eax,%gs:0x12345678\n"
			"addl	$0x1234, %ecx\n"
			"orl	$0x1234, %ecx\n"
			"testl 0x4(%ebx, %edx, 4), %ecx\n"
			"testl %eax, 0x4(%ebx, %edx, 4)\n"
			"testw %ax, 0x4(%ebx, %edx, 4)\n"
			"xchgl %eax, %ebx\n"
			"xchgl %eax, (%ebx)\n"
			"nop\n"

			"movb %al, 0x78563412\n"
			"movb 0x78563412,%al\n"
			"movw %ax, 0x78563412\n"
			"movw 0x78563412,%ax\n"
			"movl %eax, 0x78563412\n"
			"movl 0x78563412,%eax\n"

	"repnz movsb\n"
	"repnz movsw\n"
	"repnz movsl\n"

	"repz cmpsb\n"
	"repz cmpsw\n"
	"repz cmpsl\n"


	"movb $0x12, %ah\n"
	"movb $0x12, %al\n"
	"movb $0x12, %bh\n"
	"movb $0x12, %bl\n"
	"movb $0x12, %ch\n"
	"movb $0x12, %cl\n"
	"movb $0x12, %dh\n"
	"movb $0x12, %dl\n"

			"pusha\n"
			"popa\n"

			"1:\n"
			"jz 1b\n"
		);
}

int main()
{
	uint8_t * ptr = (uint8_t*)func;
	uint8_t * prev_ptr = ptr;
	int i = 0, j = 0;
	do {
		printf("%p:%d: ", ptr, i++);
		ptr = next_inst(ptr);
	} while (ptr != NULL);


	ptr = (uint8_t*)func2;
	prev_ptr = ptr;
	i = 0, j = 0;
	do {
		printf("%p:%d: ", ptr, i++);
		ptr = next_inst(ptr);
	} while (ptr != NULL);



	return 0;
}

// vim:ts=4:sw=4

