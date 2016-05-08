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
asm volatile(
			 "__interrupt_wrap:;"
			 "push %rax;"
			 "push %rcx;"
			 
			 "mov 16(%rsp), %rax;"
			 "mov 8(%rsp), %rcx;"
			 "mov %rcx, 16(%rsp);"
			 "mov 0(%rsp), %rcx;"
			 "mov %rcx, 8(%rsp);"
			 "add $8, %rsp;"
			 "mov %rsp, %rcx;"
			 "add $16, %rcx;"
			 
			 "push %rdx;"
			 "push %rbx;"
			 "push %rbp;"
			 "push %rsi;"
			 "push %rdi;"
			 "push %r8;"
			 "push %r9;"
			 "push %r10;"
			 "push %r11;"
			 "push %r12;"
			 "push %r13;"
			 "push %r14;"
			 "push %r15;"
			 
			 "mov %rcx,%rsi;"
			 "mov %rax,%rdi;"
			 "call interrupt_handler;"
			 
			 "popq %r15;"
			 "popq %r14;"
			 "popq %r13;"
			 "popq %r12;"
			 "popq %r11;"
			 "popq %r10;"
			 "popq %r9;"
			 "popq %r8;"
			 "popq %rdi;"
			 "popq %rsi;"
			 "popq %rbp;"
			 "popq %rbx;"
			 "popq %rdx;"
			 "popq %rcx;"
			 "movb $0x20, %al;"
			 "outb %al, $0x20;"
			 "popq %rax;"
			 
			 "iretq;"
			 ".align 16");
extern "C" {
	void volatile __attribute__((sysv_abi)) interrupt_handler(uint64_t intr,
															  uint64_t stack);
}
void volatile __attribute__((sysv_abi)) interrupt_handler(uint64_t intr,
														  uint64_t stack) {
	Interrupts::handle(intr, stack);
}
void Interrupts::handle(unsigned char intr, uint64_t stack) {
	uint64_t *rsp = (uint64_t*)stack;
	if (intr < 0x20) {
		printf("\nKernel fault #%02xh\nStack print:\n", intr);
		printf("RSP=%016ph\n", rsp);
		for(int i = 0; i < 7; i++) printf("%016x\n", rsp[i]);
		for(;;) asm("hlt");
	} else if (intr == 0x21) {
		printf("KBD %02xh\n", inportb(0x60));
	} else if (intr != 0x20) {
		printf("INT %02xh\n", intr);
	}
	callback_locks[intr].lock();
	intcbreg *reg = callbacks[intr];
	intcb *cb = (reg != 0) ? reg->cb : 0;
	callback_locks[intr].release();
	while (reg != 0) {
		if (cb != 0) cb();
		callback_locks[intr].lock();
		reg = reg->next;
		cb = (reg != 0) ? reg->cb : 0;
		callback_locks[intr].release();
	}
	ACPI::EOI();
}
extern "C" void *__interrupt_wrap;
void Interrupts::init() {
	idt = (PIDT)Memory::alloc(sizeof(IDT), 0x1000);
	idt->rec.limit = sizeof(idt->ints) -1;
	idt->rec.addr = &idt->ints[0];
	handlers = (int_handler*)Memory::alloc(sizeof(int_handler)*256, 0x1000);
	void* addr = (void*)&__interrupt_wrap;
	for(int i = 0; i < 256; i++) {
		uintptr_t jmp_from = (uintptr_t)&(handlers[i].reljmp);
		uintptr_t jmp_to = (uintptr_t)addr;
		uintptr_t diff = jmp_to - jmp_from - 5;
		handlers[i].push = 0x68;
		handlers[i].int_num = i;
		handlers[i].reljmp = 0xE9;
		handlers[i].diff = diff;
		
		uintptr_t hptr = (uintptr_t)(&handlers[i]);
		
		idt->ints[i].zero = 0;
		idt->ints[i].reserved = 0;
		idt->ints[i].selector = 8;
		idt->ints[i].type = 0x8E;  // P[7]=1, DPL[65]=0, S[4]=0, Type[3210] = E
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
		static const uint16_t rld = 0x000F;
		outportb(0x40, rld & 0xFF);
		outportb(0x40, (rld >> 8) & 0xFF);
	}
	maskIRQ(0);
}

void Interrupts::maskIRQ(uint16_t mask) {
	outportb(0x21, mask & 0xFF);
	outportb(0xA1, (mask >> 8) & 0xFF);
}

void Interrupts::loadVector() {
	asm volatile("lidtq %0\nsti"::"m"(idt->rec));
}

uint16_t Interrupts::getIRQmask() {
	return inportb(0x21) | (inportb(0xA1) << 8);
}

void Interrupts::addCallback(uint8_t intr, intcb* cb) {
	intcbreg *reg = (intcbreg*)Memory::alloc(sizeof(intcbreg));
	reg->cb = cb;
	reg->next = 0;
	reg->prev = 0;
	
	asm volatile("pushfq; cli");
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
	asm volatile("popfq");
}
