//    PhoeniX OS Interrupts subsystem
//    Copyright (C) 2013  PhoeniX
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "interrupts.hpp"
PIDT idt = 0;
char* handlers = 0;
INTERRUPT32 interrupts32[256];
bool ints_set = 0;
asm("\
_interrupt_handler:\n\
\
push %rax\n\
push %rcx\n\
mov 16(%rsp), %ax\n\
movq 8(%rsp), %rcx\n\
movq %rcx,10(%rsp)\n\
movq 0(%rsp), %rcx\n\
movq %rcx,2(%rsp)\n\
add $2, %rsp\n\
movq %rsp, %rcx\n\
addq $16, %rcx\n\
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
movq %rcx,%rsi\n\
movl %eax,%edi\n\
call interrupt_handler\n\
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
movb $0x20, %al\n\
outb %al, $0x20\n\
popq %rax\n\
\
iretq\n\
.align 16\n\
interrupt_handler:\
");
void interrupt_handler(unsigned char intr, _uint64 stack){
	if (!ints_set) return;
	_int64 *rsp = (_int64*)stack;
	if(intr<0x20){
		print("\nKernel fault #"); printb(intr); print("h\nStack print:\n");
		print("RSP=");printq((_uint64)rsp); print("h\n");
		for(int i=0;i<7;i++)
			{printq(rsp[i]); print("\n");}
		for(;;);
	} else if(intr != 0x20) {
		print("INT "); prints(intr); print("h\n");
	}
}
void interrupts_init()
{
	idt = (PIDT)malloc(sizeof(IDT),0x1000);
	idt->rec.limit = sizeof(idt->ints) -1;
	idt->rec.addr = &idt->ints[0];
	handlers = (char*)malloc(9*256,0x1000);
	void* addr;
	asm("mov $_interrupt_handler,%q0":"=a"(addr));
	for(int i=0; i<256; i++){
		_uint64 jmp_from = (_uint64)&handlers[9*i+ 4];
		_uint64 jmp_to = (_uint64)addr;
		_uint64 diff = jmp_to - jmp_from - 5;
		handlers[9*i+ 0] = 0x66;				// push
		handlers[9*i+ 1] = 0x68;				// short
		handlers[9*i+ 2] = i & 0xFF;			// int_num [2]
		handlers[9*i+ 3] = 0x00;
		handlers[9*i+ 4] = 0xE9;				// jmp rel
		handlers[9*i+ 5] = (diff      ) & 0xFF;
		handlers[9*i+ 6] = (diff >>  8) & 0xFF;
		handlers[9*i+ 7] = (diff >> 16) & 0xFF;
		handlers[9*i+ 8] = (diff >> 24) & 0xFF;
		
		idt->ints[i].zero = 0;
		idt->ints[i].reserved = 0;
		idt->ints[i].selector = 8;
		idt->ints[i].type = 0x8E;	// P[7]=1, DPL[65]=0, S[4]=0, Type[3210] = E
		idt->ints[i].offset_low = ((_int64)(&handlers[9*i]) >> 0) & 0xFFFF;
		idt->ints[i].offset_middle = ((_int64)(&handlers[9*i]) >> 16) & 0xFFFF;
		idt->ints[i].offset_high = ((_int64)(&handlers[9*i]) >> 32) & 0xFFFFFFFF;
	}

	outportb(0x20, 0x11);
	outportb(0xA0, 0x11);
	outportb(0x21, 0x20);
	outportb(0xA1, 0x28);
	outportb(0x21, 0x04);
	outportb(0xA1, 0x02);
	outportb(0x21, 0x01);
	outportb(0xA1, 0x01);

	outportb(0x21, 0);
	outportb(0xA1, 0);
    interrupt_handler(0,0);
    ints_set = 1;
	asm volatile( "lidtq %0\nsti"::"m"(idt->rec));
}