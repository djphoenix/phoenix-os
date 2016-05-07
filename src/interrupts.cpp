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
PIDT Interrupts::idt = 0;
intcbreg *Interrupts::callbacks[256];
Mutex Interrupts::callback_locks[256];
int_handler* Interrupts::handlers = 0;
INTERRUPT32 Interrupts::interrupts32[256];
asm volatile ("\
	__interrupt_wrap:\n\
	\
	push %rax\n\
	push %rcx\n\
	\
	mov 16(%rsp), %rax\n\
	mov 8(%rsp), %rcx\n\
	mov %rcx, 16(%rsp)\n\
	mov 0(%rsp), %rcx\n\
	mov %rcx, 8(%rsp)\n\
	add $8, %rsp\n\
	mov %rsp, %rcx\n\
	add $16, %rcx\n\
	\
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
	mov %rcx,%rsi\n\
	mov %rax,%rdi\n\
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
	.align 16\
");
extern "C" {
	void volatile __attribute__((sysv_abi)) interrupt_handler(_uint64 intr, _uint64 stack);
}
void volatile __attribute__((sysv_abi)) interrupt_handler(_uint64 intr, _uint64 stack){
	Interrupts::handle(intr,stack);
}
void Interrupts::handle(unsigned char intr, _uint64 stack){
	_int64 *rsp = (_int64*)stack;
	if(intr<0x20){
		print("\nKernel fault #"); printb(intr); print("h\nStack print:\n");
		print("RSP=");printq((_uint64)rsp); print("h\n");
		for(int i=0;i<7;i++)
		{printq(rsp[i]); print("\n");}
		for(;;);
	} else if(intr == 0x21) {
		print("KBD "); printb(inportb(0x60)); print("\n");
	} else if(intr != 0x20) {
		print("INT "); prints(intr); print("h\n");
	}
	callback_locks[intr].lock();
	intcbreg *reg = callbacks[intr];
	intcb *cb;
	if (reg != 0) cb = reg->cb;
	callback_locks[intr].release();
	while (reg != 0) {
		if (cb != 0) cb();
		callback_locks[intr].lock();
		reg = reg->next;
		if (reg != 0) cb = reg->cb;
		callback_locks[intr].release();
	};
	ACPI::EOI();
}
void Interrupts::init()
{
	idt = (PIDT)Memory::alloc(sizeof(IDT),0x1000);
	idt->rec.limit = sizeof(idt->ints) -1;
	idt->rec.addr = &idt->ints[0];
	handlers = (int_handler*)Memory::alloc(sizeof(int_handler)*256,0x1000);
	void* addr;
	asm("movabs $__interrupt_wrap,%q0":"=a"(addr));
	for(int i=0; i<256; i++){
		_uint64 jmp_from = (_uint64)&(handlers[i].reljmp);
		_uint64 jmp_to = (_uint64)addr;
		_uint64 diff = jmp_to - jmp_from - 5;
		handlers[i].push = 0x68;
		handlers[i].int_num = i;
		handlers[i].reljmp = 0xE9;
		handlers[i].diff = diff;
		
		_uint64 hptr = (_uint64)(&handlers[i]);
		
		idt->ints[i].zero = 0;
		idt->ints[i].reserved = 0;
		idt->ints[i].selector = 8;
		idt->ints[i].type = 0x8E;	// P[7]=1, DPL[65]=0, S[4]=0, Type[3210] = E
		idt->ints[i].offset_low = (hptr >> 0) & 0xFFFF;
		idt->ints[i].offset_middle = (hptr >> 16) & 0xFFFF;
		idt->ints[i].offset_high = (hptr >> 32) & 0xFFFFFFFF;
		
		callbacks[i] = 0;
		callback_locks[i] = Mutex();
	}
	
	outportb(0x20, 0x11);
	outportb(0xA0, 0x11);
	outportb(0x21, 0x20);
	outportb(0xA1, 0x28);
	outportb(0x21, 0x04);
	outportb(0xA1, 0x02);
	outportb(0x21, 0x01);
	outportb(0xA1, 0x01);
	
	loadVector();
	
	if (!(ACPI::getController())->initAPIC()) {
		outportb(0x43, 0x34);
		static const int rld = 0x000F;
		outportb(0x40, rld & 0xFF);
		outportb(0x40, (rld >> 8) & 0xFF);
	}
	maskIRQ(0);
}

void Interrupts::maskIRQ(unsigned short mask){
	outportb(0x21, mask & 0xFF);
	outportb(0xA1, (mask >> 8) & 0xFF);
}

void Interrupts::loadVector(){
	asm volatile( "lidtq %0\nsti"::"m"(idt->rec));
}

unsigned short Interrupts::getIRQmask(){
	return inportb(0x21) | (inportb(0xA1) << 8);
}

void Interrupts::addCallback(unsigned char intr, intcb* cb){
	asm volatile("cli");
	
	intcbreg *reg = (intcbreg*)Memory::alloc(sizeof(intcbreg));
	reg->cb = cb;
	reg->next = 0;
	reg->prev = 0;
	
	callback_locks[intr].lock();
	intcbreg *last = callbacks[intr];
	if (last == 0) {
		callbacks[intr] = reg;
	} else {
		while (last->next != 0) last = last->next;
		reg->prev = last;
		last->next = reg;
	}
	callback_locks[intr].release();
	asm volatile("sti");
}
