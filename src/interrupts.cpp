#include "interrupts.h"
PIDT idt = 0;
PHCODE handlers = 0;
asm volatile("\
_interrupt_handler:\n\
\
push %rax\n\
mov 8(%esp), %ax\n\
push %rcx\n\
movq %rsp, %rcx\n\
addq $18, %rcx\n\
push %rdx\n\
push %rbx\n\
push %rbp\n\
push %rsi\n\
push %rdi\n\
push %r8\n\
push %r9\n\
push %r10\n\
push %r11\n\
push %r12\n\
push %r13\n\
push %r14\n\
push %r15\n\
\
call interrupt_handler\n\
movb $0x20, %al\n\
outb %al, $0x20\n\
\
popq %r15\n\
popq %r14\n\
popq %r13\n\
popq %r12\n\
popq %r11\n\
popq %r10\n\
popq %r9\n\
popq %r8\n\
popq %rdi\n\
popq %rsi\n\
popq %rbp\n\
popq %rbx\n\
popq %rdx\n\
popq %rcx\n\
popq %rax\n\
\
add $2, %esp\n\
iretq\n\
.align 16\n\
interrupt_handler:\
");
void interrupt_handler(){
	unsigned char al; asm("mov %%al, %b0":"=b"(al));
	_int64 *rsp; asm("mov %%rsp, %q0":"=q"(rsp));
	if(al<0x20){
		print("int "); printb(al); print("h\n");
		for(int i=0;i<10;i++)
			{printq(rsp[i]); print("\n");}
		for(;;);
	} else {
		if(al == 0x21) { // Keyboard
			print("int "); printb(al); print("h");
			print(" - "); printb(inportb(0x60)); print("h");
			outportb(0x61, inportb(0x61) | 1);
			print("\n");
		}
	}
}
void interrupts_init()
{
	idt = (PIDT)malloc(sizeof(IDT));
	idt->rec.limit = sizeof(idt->ints) -1;
	idt->rec.addr = &idt->ints[0];
	handlers = (PHCODE)malloc(sizeof(HCODE)*256);
	void* addr;
	asm("mov $_interrupt_handler,%q0":"=a"(addr));
	for(int i=0; i<256; i++){
		handlers[i].push_w = 0x6866;
		handlers[i].intr = i & 0xFF;
		handlers[i].push = 0x68;
		handlers[i].addr = (_int64)addr;
		handlers[i].ret = 0xC3;
		
		idt->ints[i].zero = 0;
		idt->ints[i].reserved = 0;
		idt->ints[i].selector = 8;
		idt->ints[i].type = 0x8E;	// P[7]=1, DPL[65]=0, S[4]=0, Type[3210] = E
		idt->ints[i].offset_low = ((_int64)(&handlers[i]) >> 0) & 0xFFFF;
		idt->ints[i].offset_middle = ((_int64)(&handlers[i]) >> 16) & 0xFFFF;
		idt->ints[i].offset_high = ((_int64)(&handlers[i]) >> 32) & 0xFFFFFFFF;
	}

	outportb(0x20, 0x11);
	outportb(0xA0, 0x11);
	outportb(0x21, 0x20);
	outportb(0xA1, 0x28);
	outportb(0x21, 0x04);
	outportb(0xA1, 0x02);
	outportb(0x21, 0x01);
	outportb(0xA1, 0x01);
	outportb(0x21, 0xFC);
	outportb(0xA1, 0xFF);
	asm volatile( "lidtq %0\nsti"::"m"(idt->rec));
}