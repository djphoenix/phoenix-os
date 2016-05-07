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

#ifndef INTERRUPTS_H
#define INTERRUPTS_H
#include "pxlib.hpp"
#include "memory.hpp"
#include "acpi.hpp"
#pragma pack(push,1)
typedef struct {
	short offset_low;
	short selector;
	char zero;
	char type;
	short offset_middle;
} INTERRUPT32, *PINTERRUPT32;
typedef struct {
	short offset_low;
	short selector;
	char zero;
	char type;
	short offset_middle;
	uint offset_high;
	uint reserved;
} INTERRUPT64, *PINTERRUPT64;
typedef struct {
	short limit;
	void* addr;
} IDTR, *PIDTR;
#pragma pack(pop)
typedef struct {
	INTERRUPT64 ints[256];
	IDTR rec;
} IDT, *PIDT;

typedef void intcb();
typedef struct {unsigned char intr; intcb *cb;} intcbreg;

#pragma pack(push,1)
struct int_handler {
	// 68 04 03 02 01  pushq  ${int_num}
	// e9 46 ec 3f 00  jmp . + {diff}
	unsigned char push; // == 0x68
	unsigned int int_num;
	unsigned char reljmp; // == 0xE9
	unsigned int diff;
};
#pragma pack(pop)

class Interrupts {
private:
	static intcbreg* callbacks;
	static int_handler* handlers;
	static PIDT idt;
public:
	static INTERRUPT32 interrupts32[256];
	static void init();
	static void handle(unsigned char intr, _uint64 stack);
	static void maskIRQ(unsigned short mask);
	static unsigned short getIRQmask();
	static void addCallback(unsigned char intr, intcb* cb);
	static void loadVector();
};
#endif
