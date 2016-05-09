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

#pragma once
#include "pxlib.hpp"
#include "memory.hpp"
#include "acpi.hpp"
#pragma pack(push,1)
typedef struct {
	uint16_t offset_low;
	uint16_t selector;
	uint8_t zero;
	uint8_t type;
	uint16_t offset_middle;
} INTERRUPT32, *PINTERRUPT32;
typedef struct {
	uint16_t offset_low;
	uint16_t selector;
	uint8_t ist:3;
	uint8_t rsvd1:5;
	uint8_t type:4;
	uint8_t rsvd2:1;
	uint8_t dpl:2;
	bool present:1;
	uint16_t offset_middle;
	uint32_t offset_high;
	uint32_t rsvd3;
} INTERRUPT64, *PINTERRUPT64;
typedef struct {
	uint16_t limit;
	void* addr;
} IDTR, *PIDTR;
struct int_handler {
	// 68 04 03 02 01  pushq  ${int_num}
	// e9 46 ec 3f 00  jmp . + {diff}
	uint8_t push; // == 0x68
	uint32_t int_num;
	uint8_t reljmp; // == 0xE9
	uint32_t diff;
};
#pragma pack(pop)
typedef struct {
	INTERRUPT64 ints[256];
	IDTR rec;
} IDT, *PIDT;

typedef void intcb();
struct _intcbreg;
typedef struct _intcbreg {
	intcb *cb;
	_intcbreg *prev, *next;
} intcbreg;

class Interrupts {
private:
	static intcbreg *callbacks[256];
	static Mutex callback_locks[256];
	static Mutex fault;
	static int_handler* handlers;
	static PIDT idt;
public:
	static INTERRUPT32 interrupts32[256];
	static void init();
	static uint64_t handle(uint8_t intr, uint64_t stack);
	static void maskIRQ(uint16_t mask);
	static void setIST(uint8_t ist);
	static uint16_t getIRQmask();
	static void addCallback(uint8_t intr, intcb* cb);
	static void loadVector();
};
