#include "interrupts.h"
PIDT idt = 0;
asm("\
_interrupt_handler:\n\
movq $0xB8000, %rax\n\
movb $'A', (%rax)\n\
jmp _interrupt_handler\n\
push %ax\n\
push %cx\n\
push %dx\n\
push %bx\n\
push %sp\n\
push %bp\n\
push %si\n\
push %di\n\
call interrupt_handler\n\
popw %di\n\
popw %si\n\
popw %bp\n\
popw %bx\n\
popw %bx\n\
popw %dx\n\
popw %cx\n\
popw %ax\n\
iretq\n\
interrupt_handler:\
");
void interrupt_handler(){
}
void interrupts_init()
{
	print("Initializing interrupts...");
	idt = (PIDT)malloc(sizeof(IDT));
	idt->rec.limit = sizeof(idt->ints) -1;
	idt->rec.addr = &idt->ints[0];
	void* addr;
	asm("mov $_interrupt_handler,%q0":"=a"(addr));
	for(int i=0; i<256; i++){
		idt->ints[i].zero = 0;
		idt->ints[i].reserved = 0;
		idt->ints[i].selector = 8;
		idt->ints[i].type = 0x0E;	// P[7]=0, DPL[65]=0, S[4]=0, Type[3210] = E
		idt->ints[i].offset_low = ((_int64)addr >> 0) & 0xFFFF;
		idt->ints[i].offset_middle = ((_int64)addr >> 16) & 0xFFFF;
		idt->ints[i].offset_high = ((_int64)addr >> 32) & 0xFFFFFFFF;
	}
 print("\n");
	printl(*(_int64*)((_int64)(&idt->ints[0])+12)); print("\n");
	printl(*(_int64*)((_int64)(&idt->ints[0])+8)); print("\n");
	printl(*(_int64*)((_int64)(&idt->ints[0])+4)); print("\n");
	printl(*(_int64*)(&idt->ints[0])); print("\n");

	outportb(0x20, 0x11);
	outportb(0xA0, 0x11);
	outportb(0x21, 0x20);
	outportb(0xA1, 0x28);
	outportb(0x21, 0x04);
	outportb(0xA1, 0x02);
	outportb(0x21, 0x01);
	outportb(0xA1, 0x01);
	outportb(0x21, 0x0);
	outportb(0xA1, 0x0);
	asm volatile( "lidtq %0\nsti"::"m"(idt->rec));
}