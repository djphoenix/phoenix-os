//    PhoeniX OS System calls subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "syscall.hpp"
#include "processmanager.hpp"
#include "pagetable.hpp"

#include "./syscall_hash.hpp"
#include "./syscall_setup.hpp"

static void syscall_puts(uintptr_t strptr) {
  Process *process = ProcessManager::getManager()->currentProcess();
  ptr<char> str(process->readString(strptr));
  klib::puts(str.get());
}

static void syscall_exit(int code) {
  ProcessManager *manager = ProcessManager::getManager();
  Process *process = manager->currentProcess();
  void *rsp; asm volatile("mov %%rsp, %q0; and $~0xFFF, %q0":"=r"(rsp));
  Pagetable::Entry *pt; asm volatile("mov %%cr3, %q0":"=r"(pt));
  Pagetable::Entry *pte = Pagetable::Entry::find(rsp, pt);
  asm volatile(
      "callq _ZN7Process4exitEi;"
      "sti; movq $0, %q2; xor %%rbp, %%rbp;"
      "jmp _ZN14ProcessManager12process_loopEv"
      ::"D"(process), "S"(code), "a"(pte) : "cc"
      );
}

static void syscall_kread(void *out, const void *kaddr, size_t size) {
  if (kaddr == nullptr) return;
  Process *process = ProcessManager::getManager()->currentProcess();
  for (size_t p = 0; p < size; p += 0x1000) {
    Pagetable::map(reinterpret_cast<const void*>(uintptr_t(kaddr) + p), Pagetable::MemoryType::DATA_RO);
  }
  process->writeData(uintptr_t(out), kaddr, size);
}

static void syscall_ioprovide(const char *path, const void *modptr) {
  Process *process = ProcessManager::getManager()->currentProcess();
  ptr<char> str(process->readString(uintptr_t(path)));
  printf("ioprovide [%s(%#lx) / %p] [%s]\n", process->getName(), process->getId(), modptr, str.get());
}

#define SYSCALL_ENT(name) { \
  syscall_hash_ce(#name), \
  reinterpret_cast<void*>(syscall_ ## name) \
}

static const volatile struct {
  uint64_t hash;
  void *entry;
} PACKED syscall_map[] = {
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

      // Set stack to absolute address
      "mov %rsp, %r12; shr $39, %r12; and $0x1FF, %r12;" // r12 = ptx
      "mov (%rbx,%r12,8), %r13; and $~0xFFF, %r13; btr $63, %r13;" // rbx = pt, r12 = pte

      "mov %rsp, %r12; shr $30, %r12; and $0x1FF, %r12;" // r12 = pdx
      "mov (%r13,%r12,8), %r13; and $~0xFFF, %r13; btr $63, %r13;" // r13 = pde

      "mov %rsp, %r12; shr $21, %r12; and $0x1FF, %r12;" // r12 = pdpx
      "mov (%r13,%r12,8), %r13; and $~0xFFF, %r13; btr $63, %r13;" // r13 = pdpe

      "mov %rsp, %r12; shr $12, %r12; and $0x1FF, %r12;" // r12 = pml4x
      "mov (%r13,%r12,8), %r13; and $~0xFFF, %r13; btr $63, %r13;" // r13 = pml4e

      // Save stack
      "mov %rsp, %r12;"
      "mov %rsp, %r14; and $0xFFF, %r14; add %r14, %r13; mov %r13, %rsp;"

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
