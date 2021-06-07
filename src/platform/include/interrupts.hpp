//    PhoeniX OS Interrupts subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"
#include "heap.hpp"
#include "list.hpp"

struct DTREG {
  uint16_t limit;
  void* addr;
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

struct GDT {
  struct Entry {
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

    constexpr Entry() :
        seg_lim_low(0), base_low(0), type(0), system(0), dpl(0), present(0),
        seg_lim_high(0), avl(0), islong(0), db(0), granularity(0), base_high(0) {}

    constexpr Entry(
        uint64_t base, uint64_t limit,
        uint8_t type, uint8_t dpl,
        bool system, bool present, bool avl, bool islong,
        bool db, bool granularity):
            seg_lim_low(limit & 0xFFFF), base_low(base & 0xFFFFFF),
            type(type), system(system), dpl(dpl), present(present),
            seg_lim_high((limit >> 16) & 0xF), avl(avl), islong(islong), db(db),
            granularity(granularity), base_high((base >> 24) & 0xFF) {}
  } PACKED;

  struct SystemEntry {
    Entry ent;
    uint64_t base_high :32;
    uint32_t rsvd;

    uint64_t getBase() { return (uintptr_t(base_high) << 32) | ent.getBase(); }

    void setBase(uint64_t base) {
      ent.setBase(base);
      base_high = (base >> 32);
    }

    constexpr SystemEntry(): ent(), base_high(0), rsvd(0) { }

    constexpr SystemEntry(uint64_t base, uint64_t limit, uint8_t type, uint8_t dpl,
                bool system, bool present, bool avl, bool islong, bool db,
                bool granularity) :
        ent(base, limit, type, dpl, system, present, avl, islong, db,
            granularity), base_high(base >> 32), rsvd(0) { }
  } PACKED;

  Entry ents[5];
  SystemEntry sys_ents[];

  static size_t size(size_t sys_count) {
    return sizeof(Entry) * 5 + sizeof(SystemEntry) * sys_count;
  }
} PACKED;

class Process;
class Interrupts {
 public:
  struct CallbackRegs {
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
    struct SSE {
      uint64_t sse[64];
      uint64_t mxcsr;
    } __attribute__((aligned(16))) sse;
  } __attribute__((aligned(16)));
  typedef bool Callback(uint8_t intr, uint32_t code, CallbackRegs *regs);

  struct REC32 {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type;
    uint16_t offset_middle;
  } PACKED;

  struct REC64 {
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

    constexpr REC64():
      offset_low(0), selector(0), ist(0), rsvd1(0), type(0), rsvd2(0), dpl(0),
      present(0), offset_middle(0), offset_high(0), rsvd3(0) {}
    constexpr REC64(uint64_t offset, uint16_t selector, uint8_t ist, uint8_t type,
                uint8_t dpl, bool present) :
        offset_low(uint16_t(offset)), selector(selector), ist(ist), rsvd1(0),
        type(type), rsvd2(0), dpl(dpl), present(present),
        offset_middle(uint16_t(offset >> 16)), offset_high(offset >> 32), rsvd3(0) {}
  } PACKED;

 private:
  struct Handler;

  static uintptr_t eoi_vector;
  static List<Callback*> *callbacks;
  static Mutex callback_locks[256];
  static Mutex fault;
  static Handler* handlers;
  static REC64 *idt;
  static GDT *gdt;
  static void init();
  static uint64_t handle(uint8_t intr, uint64_t stack, uint64_t *cr3, uint64_t *sse);

 public:
  static void print(uint8_t num, CallbackRegs *regs, uint32_t code, const Process *process = nullptr);
  static void maskIRQ(uint16_t mask);
  static uint16_t getIRQmask();
  static void addCallback(uint8_t intr, Callback* cb);
  static void loadVector();
};
