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
typedef struct {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t zero;
  uint8_t type;
  uint16_t offset_middle;
}__attribute__((packed)) INTERRUPT32, *PINTERRUPT32;
typedef struct {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t ist :3;
  uint8_t rsvd1 :5;
  uint8_t type :4;
  uint8_t rsvd2 :1;
  uint8_t dpl :2;
  bool present :1;
  uint16_t offset_middle;
  uint32_t offset_high;
  uint32_t rsvd3;
}__attribute__((packed)) INTERRUPT64, *PINTERRUPT64;
typedef struct {
  uint16_t limit;
  void* addr;
}__attribute__((packed)) IDTR, *PIDTR;
struct int_handler {
  // 68 04 03 02 01  pushq  ${int_num}
  // e9 46 ec 3f 00  jmp . + {diff}
  uint8_t push;  // == 0x68
  uint32_t int_num;
  uint8_t reljmp;  // == 0xE9
  uint32_t diff;
}__attribute__((packed));
typedef struct {
  INTERRUPT64 ints[256];
  IDTR rec;
} IDT, *PIDT;

typedef struct {
  uint32_t cpuid;
  uint64_t cr3;
  uint64_t rip;
  uint16_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint16_t ss;
  uint8_t dpl;
  uint64_t rax, rcx, rdx, rbx;
  uint64_t rbp, rsi, rdi;
  uint64_t r8, r9, r10, r11;
  uint64_t r12, r13, r14, r15;
} intcb_regs;

typedef bool
intcb(uint32_t intr, intcb_regs *regs);
struct _intcbreg;
typedef struct _intcbreg {
  intcb *cb;
  _intcbreg *prev, *next;
} intcbreg;

class Interrupts {
 private:
  static intcbreg *callbacks[256];
  static Mutex callback_locks[256];
  static Mutex fault, init_lock;
  static int_handler* handlers;
  static PIDT idt;
 public:
  static INTERRUPT32 interrupts32[256];
  static void init();
  static uint64_t handle(uint8_t intr, uint64_t stack, uint64_t *cr3);
  static void maskIRQ(uint16_t mask);
  static uint16_t getIRQmask();
  static void addCallback(uint8_t intr, intcb* cb);
  static void loadVector();
};
