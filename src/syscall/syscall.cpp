//    PhoeniX OS System calls subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "syscall.hpp"
#include "scheduler.hpp"
#include "pagetable.hpp"
#include "process.hpp"

#include "syscall_hash.hpp"
#include "syscall_setup.hpp"

#include "sprintf.hpp"
#include "kprint.hpp"

static void syscall_puts(uintptr_t strptr) {
  Process *process = Scheduler::getScheduler()->currentProcess();
  ptr<char> str(process->readString(strptr));
  kprint(str.get());
}

static void __attribute__((noreturn)) syscall_exit(int code) {
  Scheduler *scheduler = Scheduler::getScheduler();
  Process *process = scheduler->currentProcess();
  scheduler->exitProcess(process, code);
}

static void syscall_kread(void *out, const void *kaddr, size_t size) {
  if (kaddr == nullptr) return;
  Process *process = Scheduler::getScheduler()->currentProcess();
  for (size_t p = 0; p < size; p += 0x1000) {
    Pagetable::map(reinterpret_cast<const void*>(uintptr_t(kaddr) + p), Pagetable::MemoryType::DATA_RO);
  }
  process->writeData(uintptr_t(out), kaddr, size);
}

static void syscall_ioprovide(const char *path, const void *modptr) {
  Process *process = Scheduler::getScheduler()->currentProcess();
  ptr<char> str(process->readString(uintptr_t(path)));
  char printbuf[128];
  snprintf(
    printbuf, sizeof(printbuf),
    "ioprovide [%s(%#lx) / %p] [%s]\n",
    process->getName(), process->getId(), modptr, str.get()
  );
  kprint(printbuf);
}

#define SYSCALL_ENT(name) { \
  syscall_hash_ce(#name), \
  reinterpret_cast<void*>(syscall_ ## name) \
}

static const volatile struct {
  uint64_t hash;
  void *entry;
} __attribute__((packed)) syscall_map[] = {
  SYSCALL_ENT(puts),
  SYSCALL_ENT(exit),
  SYSCALL_ENT(kread),
  SYSCALL_ENT(ioprovide),
  {0, nullptr}
};

#undef SYSCALL_ENT

static uint64_t syscall_index(uint64_t hash) {
  static const size_t num = sizeof(syscall_map) / sizeof(*syscall_map);
  for (uint64_t idx = 0; idx < num - 1; idx++) {
    if (syscall_map[idx].hash == hash) return idx;
  }
  return static_cast<uint64_t>(-1);
}

uint64_t Syscall::callByName(const char *name) {
  uint64_t hash = syscall_hash(name);
  if (syscall_index(hash) == uint64_t(-1)) return 0;
  return hash;
}

void __attribute((naked)) Syscall::wrapper() {
  asm volatile(
      // Save registers
      "push %rbx;"
      "push %r11;"
      "push %r12;"
      "push %r13;"
      "push %r14;"
      "push %r15;"

      // Save pagetable
      "mov %cr3, %rbx;"

      "subq $16, %rsp;"
      "str (%rsp); mov (%rsp), %r12; and $0xFFFF, %r12;" // TR=r12
      "sgdt (%rsp);"

      "mov 2(%rsp), %r13; add %r12, %r13; mov %r13, (%rsp);" // GDT:SE(TSS64)=r13

      "mov  (%r13), %r12; shr $16, %r12; and $0x00FFFFFF, %r12; mov %r12, 8(%rsp);"
      "mov  (%r13), %r12; shr $56, %r12; and $0xFF, %r12; shl $24, %r12; orq %r12, 8(%rsp);"
      "mov 8(%r13), %r12; shl $32, %r12; orq 8(%rsp), %r12;" // TSS64=r12
      "mov 36(%r12), %r12;" // IST=r12

      "add $16, %rsp;"
      "xchg %r12, %rsp;"

      // Set kernel pagetable
      "_wrapper_mov_cr3: movabsq $0, %r13; mov %r13, %cr3;"

      // Align stack
      "and $(~0x0F), %rsp;"

      "sub $0x200, %rsp;"
      "fxsave (%rsp);"

      // Find syscall handler
      "lea _ZL11syscall_map(%rip), %r13;"
      "1:"
      "cmp %rax, (%r13);"
      "je 1f;"
      "cmpq $0, (%r13);"
      "je 2f;"
      "add $16, %r13;"
      "jmp 1b;"

      "1:"
      "mov %rcx, %r14; callq *8(%r13); mov %r14, %rcx;"

      "fxrstor (%rsp);"

      // Restore process stack & pagetable
      "mov %r12, %rsp; mov %rbx, %cr3;"

      // Restore registers
      "pop %r15;"
      "pop %r14;"
      "pop %r13;"
      "pop %r12;"
      "pop %r11;"
      "pop %rbx;"

      // Return to ring3
      "sysretq;"
      "2: ud2;"
  );
}

void Syscall::setup() {
  // Patch kernel CR3 address
  Pagetable::map(reinterpret_cast<const void*>(wrapper), Pagetable::MemoryType::CODE_RW);
  asm volatile("mov %%cr3, %%rax; mov %%rax, 2 + _wrapper_mov_cr3(%%rip)":::"rax");
  Pagetable::map(reinterpret_cast<const void*>(wrapper), Pagetable::MemoryType::CODE_RX);
  wrmsr(MSR_STAR, uint64_t(0x10) << 48 | uint64_t(0x8) << 32);
  wrmsr(MSR_LSTAR, uintptr_t(wrapper));
  wrmsr(MSR_SFMASK, MSR_SFMASK_IE);
  wrmsr(MSR_EFER, rdmsr(MSR_EFER) | MSR_EFER_SCE);
}
