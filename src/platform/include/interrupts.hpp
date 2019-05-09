//    PhoeniX OS Interrupts subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"
#include "heap.hpp"
#include "list.hpp"

struct INTERRUPT32 {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t zero;
  uint8_t type;
  uint16_t offset_middle;
} PACKED;

struct INTERRUPT64 {
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

  INTERRUPT64():
    offset_low(0), selector(0), ist(0), rsvd1(0), type(0), rsvd2(0), dpl(0),
    present(0), offset_middle(0), offset_high(0), rsvd3(0) {}
  INTERRUPT64(uint64_t offset, uint16_t selector, uint8_t ist, uint8_t type,
              uint8_t dpl, bool present) :
      offset_low(offset), selector(selector), ist(ist), rsvd1(0),
      type(type), rsvd2(0), dpl(dpl), present(present),
      offset_middle(offset >> 16), offset_high(offset >> 32), rsvd3(0) {}

  ALIGNED_NEWARR(0x1000)
} PACKED;

struct DTREG {
  uint16_t limit;
  void* addr;
} PACKED;

struct int_handler {
  // 68 04 03 02 01  pushq  ${int_num}
  // e9 46 ec 3f 00  jmp . + {diff}
  uint8_t push;  // == 0x68
  uint32_t int_num;
  uint8_t reljmp;  // == 0xE9
  uint32_t diff;

  int_handler():
    push(0x68), int_num(0),
    reljmp(0xE9), diff(0) {}
  int_handler(uint32_t int_num, uint32_t diff):
    push(0x68), int_num(int_num),
    reljmp(0xE9), diff(diff) {}

  ALIGNED_NEWARR(0x1000)
} PACKED;

struct TSS64_ENT {
  uint32_t reserved1;
  uint64_t rsp[3];
  uint64_t reserved2;
  uint64_t ist[7];
  uint64_t reserved3;
  uint16_t reserved4;
  uint16_t iomap_base;
} PACKED;

struct GDT_ENT {
  uint64_t seg_lim_low :16;
  uint64_t base_low :24;
  uint8_t type :4;
  bool system :1;
  uint8_t dpl :2;
  bool present :1;
  uint64_t seg_lim_high :4;
  bool avl :1;
  bool islong :1;
  bool db :1;
  bool granularity :1;
  uint64_t base_high :8;

  uint64_t getBase() { return (uint64_t(base_high) << 24) | base_low; }
  uint64_t getLimit() { return (uint64_t(seg_lim_high) << 16) | seg_lim_low; }

  void setBase(uint64_t base) {
    base_low = base & 0xFFFFFF;
    base_high = (base >> 24) & 0xFF;
  }
  void setLimit(uint64_t limit) {
    seg_lim_low = limit & 0xFFFF;
    seg_lim_high = (limit >> 16) & 0xF;
  }

  GDT_ENT() :
      type(0), system(0), dpl(0), present(0), avl(0), islong(0), db(0),
      granularity(0) { setLimit(0); setBase(0); }

  GDT_ENT(
      uint64_t base, uint64_t limit,
      uint8_t type, uint8_t dpl,
      bool system, bool present, bool avl, bool islong,
      bool db, bool granularity): type(type), system(system), dpl(dpl),
          present(present), avl(avl), islong(islong), db(db),
          granularity(granularity) { setLimit(limit); setBase(base); }
} PACKED;

struct GDT_SYS_ENT {
  GDT_ENT ent;
  uint64_t base_high :32;
  uint32_t rsvd;

  uint64_t getBase() { return (uintptr_t(base_high) << 32) | ent.getBase(); }

  void setBase(uint64_t base) {
    ent.setBase(base);
    base_high = (base >> 32);
  }

  GDT_SYS_ENT(): base_high(0), rsvd(0) { }

  GDT_SYS_ENT(uint64_t base, uint64_t limit, uint8_t type, uint8_t dpl,
              bool system, bool present, bool avl, bool islong, bool db,
              bool granularity) :
      ent(base, limit, type, dpl, system, present, avl, islong, db,
          granularity), base_high(base >> 32), rsvd(0) { }
} PACKED;

struct GDT {
  GDT_ENT ents[5];
  GDT_SYS_ENT sys_ents[];

  static size_t size(size_t sys_count) {
    return sizeof(GDT_ENT) * 5 + sizeof(GDT_SYS_ENT) * sys_count;
  }
} PACKED;

struct intcb_regs {
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
};

typedef bool intcb(uint32_t intr, uint32_t code, intcb_regs *regs);

class Interrupts {
 private:
  static List<intcb*> *callbacks;
  static Mutex callback_locks[256];
  static Mutex fault;
  static int_handler* handlers;
  static INTERRUPT64 *idt;
  static GDT *gdt;
  static TSS64_ENT *tss;
  static void init();
  static uint64_t handle(uint8_t intr, uint64_t stack, uint64_t *cr3);

 public:
  static void print(uint8_t num, intcb_regs *regs, uint32_t code);
  static void maskIRQ(uint16_t mask);
  static uint16_t getIRQmask();
  static void addCallback(uint8_t intr, intcb* cb);
  static void loadVector();
};
