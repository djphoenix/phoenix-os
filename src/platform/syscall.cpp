//    PhoeniX OS System calls subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "syscall.hpp"
#include "processmanager.hpp"

#include "./syscall_hash.hpp"
#include "./syscall_setup.hpp"

static void syscall_puts(uintptr_t strptr) {
  Process *process = ProcessManager::getManager()->currentProcess();
  const char *str = process->readString(strptr);
  Display::getInstance()->write(str);
  delete str;
}

static void syscall_exit(int code) {
  ProcessManager *manager = ProcessManager::getManager();
  Process *process = manager->currentProcess();
  void *rsp; asm volatile("mov %%rsp, %q0; and $~0xFFF, %q0":"=r"(rsp));
  Pagetable::Entry *pt; asm volatile("mov %%cr3, %q0":"=r"(pt));
  Pagetable::Entry *pte = Pagetable::Entry::find(rsp, pt);
  asm volatile(
      "callq _ZN7Process4exitEi;"
      "sti; movq $0, %q2;"
      "jmp _ZN14ProcessManager12process_loopEv"
      ::"D"(process), "S"(code), "a"(pte)
      );
}

static void syscall_ioprovide(const char *path, const void *ptr) {
  Process *process = ProcessManager::getManager()->currentProcess();
  char *str = process->readString(uintptr_t(path));
  printf("ioprovide [%p] [%s]\n", ptr, str);
  delete str;
}

#define SYSCALL_ENT(name) { \
  syscall_hash(#name), \
  reinterpret_cast<void*>(syscall_ ## name) \
}

static const struct {
  uint64_t hash;
  void *entry;
} PACKED syscall_map[] = {
  SYSCALL_ENT(puts),
  SYSCALL_ENT(exit),
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
      "push %rax;"
      "push %rcx;"
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

      // Save pagetable & stack
      "mov %cr3, %rax; mov %rsp, %rbx; push %rax; push %rbx;"

      // Set stack to absolute address
      "sub $16, %rbx; ror $48, %rbx;"
      "mov $4, %rcx;"
      "1:"
      "rol $9, %rbx;"
      "mov %rbx, %rdx;"
      "and $0x1FF, %rdx;"
      "mov (%rax,%rdx,8), %rax;"
      "and $~0xFFF, %rax; btr $63, %rax;"
      "loop 1b;"
      "rol $12, %rbx; and $0xFFF, %rbx; add %rax, %rbx; mov %rbx, %rsp;"

      // Set kernel pagetable
      "_wrapper_mov_cr3: movabsq $0, %rax; mov %rax, %cr3;"

      // Find syscall handler
      "mov 128(%rsp), %rdx;"
      "lea _ZL11syscall_map(%rip), %rax;"
      "1:"
      "cmp %rdx, (%rax);"
      "je 1f;"
      "cmpq $0, (%rax);"
      "je 2f;"
      "add $16, %rax;"
      "jmp 1b;"
      "1: mov 8(%rax), %rax; callq *%rax; jmp 3f;"
      "2: ud2;"
      "3:"

      // Restore process stack & pagetable
      "pop %rcx; pop %rax; mov %rcx, %rsp; mov %rax, %cr3;"

      // Restore registers
      "pop %r15;"
      "pop %r14;"
      "pop %r13;"
      "pop %r12;"
      "pop %r11;"
      "pop %r10;"
      "pop %r9;"
      "pop %r8;"
      "pop %rdi;"
      "pop %rsi;"
      "pop %rbp;"
      "pop %rbx;"
      "pop %rdx;"
      "pop %rcx;"
      "pop %rax;"

      // Return to ring3
      "sysretq;"
  );
}

void Syscall::setup() {
  asm volatile(
      "mov %%cr3, %%rax; mov %%rax, 2 + _wrapper_mov_cr3(%%rip)":::"%rax"
      );
  wrmsr(MSR_STAR, uint64_t(0x10) << 48 | uint64_t(0x8) << 32);
  wrmsr(MSR_LSTAR, uintptr_t(wrapper));
  wrmsr(MSR_SFMASK, MSR_SFMASK_IE);
  wrmsr(MSR_EFER, rdmsr(MSR_EFER) | MSR_EFER_SCE | MSR_EFER_NXE);
}
