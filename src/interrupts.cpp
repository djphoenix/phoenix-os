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
intcbreg* Interrupts::callbacks = 0;
char* Interrupts::handlers = 0;
INTERRUPT32 Interrupts::interrupts32[256];
bool Interrupts::ints_set = 0;
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
movq %rcx,%rdx\n\
movl %eax,%ecx\n\
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
    Interrupts::handle(intr,stack);
}
void Interrupts::handle(unsigned char intr, _uint64 stack){
	if (!ints_set) return;
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
    if (callbacks) {
        _uint64 i = 0;
        while ((callbacks[i].intr != 0) || ((callbacks[i].cb != 0))) {
            if(callbacks[i].intr == intr) callbacks[i].cb();
            i++;
        }
    }
    ACPI::EOI();
}
void Interrupts::init()
{
	idt = (PIDT)Memory::alloc(sizeof(IDT),0x1000);
	idt->rec.limit = sizeof(idt->ints) -1;
	idt->rec.addr = &idt->ints[0];
	handlers = (char*)Memory::alloc(9*256,0x1000);
	void* addr;
	asm("movabs $_interrupt_handler,%q0":"=a"(addr));
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

    loadVector();

    if (!(ACPI::getController())->initAPIC()) {
        maskIRQ(0);
    }
    interrupt_handler(0,0);
    ints_set = 1;
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
    if (callbacks == 0) {
        callbacks = (intcbreg*)Memory::alloc(sizeof(intcbreg)*2);
        callbacks[0].intr = intr;
        callbacks[0].cb = cb;
        callbacks[1].intr = 0;
        callbacks[1].cb = 0;
    } else {
        _uint64 cid = 0;
        while ((callbacks[cid].intr != 0) || (callbacks[cid].cb != 0)) cid++;
        intcbreg* nc = (intcbreg*)Memory::alloc(sizeof(intcbreg)*(cid+1));
        Memory::copy(nc, callbacks, sizeof(intcbreg)*cid);
        Memory::free(callbacks);
        callbacks = nc;
        callbacks[cid].intr = intr;
        callbacks[cid].cb = cb;
        callbacks[cid+1].intr = 0;
        callbacks[cid+1].cb = 0;
    }
}
