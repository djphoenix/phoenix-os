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
Mutex Interrupts::fault;
Mutex Interrupts::init_lock = Mutex();
int_handler* Interrupts::handlers = 0;
INTERRUPT32 Interrupts::interrupts32[256];
asm volatile(
    ".global __interrupt_wrap;"
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

    "mov %rsp, %rcx;"
    "add %rax, %rcx;"
    "mov 8(%rsp), %rax;"
    "mov %rax, 8(%rcx);"
    "mov (%rsp), %rax;"
    "mov %rax, (%rcx);"
    "mov %rcx, %rsp;"

    "popq %rcx;"
    "movb $0x20, %al;"
    "outb %al, $0x20;"
    "popq %rax;"

    "iretq;"
    ".align 16");
extern "C" {
  uint64_t __attribute__((sysv_abi)) interrupt_handler(uint64_t intr,
                                                       uint64_t stack);
  extern void *__pagetable__;
  extern void *__interrupt_wrap;
}
uint64_t __attribute__((sysv_abi)) interrupt_handler(uint64_t intr,
                                                     uint64_t stack) {
  uint64_t cr3 = 0;
  asm volatile("mov %%cr3, %0":"=a"(cr3));
  asm volatile("mov %0, %%cr3"::"a"(&__pagetable__));
  uint64_t ret = Interrupts::handle(intr, stack, &cr3);
  asm volatile("mov %0, %%cr3"::"a"(cr3));
  return ret;
}

struct FAULT {
  char code[5];
  bool has_error_code;
}__attribute__((packed));

static const FAULT FAULTS[0x20] = {
  /* 00 */{ "#DE", false },
  /* 01 */{ "#DB", false },
  /* 02 */{ "#NMI", false },
  /* 03 */{ "#BP", false },
  /* 04 */{ "#OF", false },
  /* 05 */{ "#BR", false },
  /* 06 */{ "#UD", false },
  /* 07 */{ "#NM", false },

  /* 08 */{ "#DF", true },
  /* 09 */{ },
  /* 0A */{ "#TS", true },
  /* 0B */{ "#NP", true },
  /* 0C */{ "#SS", true },
  /* 0D */{ "#GP", true },
  /* 0E */{ "#PF", true },
  /* 0F */{ },

  /* 10 */{ "#MF", false },
  /* 11 */{ "#AC", true },
  /* 12 */{ "#MC", false },
  /* 13 */{ "#XM", false },
  /* 14 */{ "#VE", false },
  /* 15 */{ },
  /* 16 */{ },
  /* 17 */{ },

  /* 18 */{ },
  /* 19 */{ },
  /* 1A */{ },
  /* 1B */{ },
  /* 1C */{ },
  /* 1D */{ },
  /* 1E */{ "#SX", true },
  /* 1F */{ }
};

struct int_regs {
  uint64_t r15, r14, r13, r12;
  uint64_t r11, r10, r9, r8;
  uint64_t rdi, rsi, rbp;
  uint64_t rbx, rdx, rcx, rax;
}__attribute__((packed));
struct int_info {
  uint64_t rip, cs, rflags, rsp, ss;
}__attribute__((packed));

uint64_t Interrupts::handle(unsigned char intr, uint64_t stack, uint64_t *cr3) {
  fault.lock();
  fault.release();
  uint64_t *rsp = (uint64_t*)stack;
  bool has_code = (intr < 0x20) && FAULTS[intr].has_error_code;
  uint64_t error_code = 0;
  int_regs *regs = (int_regs*)(rsp - (12 + 3));
  if (has_code)
    error_code = *(rsp++);
  int_info *info = (int_info*)rsp;
  bool handled = false;

  uint32_t cpuid = ACPI::getController()->getCPUID();

  intcb_regs cb_regs = {
      cpuid, *cr3,

      info->rip,
      (uint16_t)(info->cs & 0xFFF8), info->rflags, info->rsp,
      (uint16_t)(info->ss & 0xFFF8), (uint8_t)(info->cs & 7),

      regs->rax,
      regs->rcx, regs->rdx, regs->rbx, regs->rbp, regs->rsi, regs->rdi,
      regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13, regs->r14,
      regs->r15 };

  callback_locks[intr].lock();
  intcbreg *reg = callbacks[intr];
  while (reg != 0 && reg->next != 0 && reg->cb == 0)
    reg = reg->next;
  intcb *cb = (reg != 0) ? reg->cb : 0;
  callback_locks[intr].release();

  while (reg != 0) {
    if (cb != 0)
      handled = cb(intr, &cb_regs);
    if (handled)
      break;
    callback_locks[intr].lock();
    reg = reg->next;
    while (reg != 0 && reg->next != 0 && reg->cb == 0)
      reg = reg->next;
    cb = (reg != 0) ? reg->cb : 0;
    callback_locks[intr].release();
  }

  if (handled) {
    *regs = {
      cb_regs.r15, cb_regs.r14, cb_regs.r13, cb_regs.r12,
      cb_regs.r11, cb_regs.r10, cb_regs.r9, cb_regs.r8,
      cb_regs.rdi, cb_regs.rsi, cb_regs.rbp,
      cb_regs.rbx, cb_regs.rdx, cb_regs.rcx, cb_regs.rax,
    };
    *info = {
      cb_regs.rip, (uint64_t)(cb_regs.cs | cb_regs.dpl),
      cb_regs.rflags | 0x202,
      cb_regs.rsp, (uint64_t)(cb_regs.ss | cb_regs.dpl)
    };
    *cr3 = cb_regs.cr3;
    ACPI::EOI();
    return has_code ? 8 : 0;
  }

  if (intr < 0x20) {
    fault.lock();
    FAULT f = FAULTS[intr];
    uint64_t cr2;
    asm volatile("mov %%cr2, %0":"=a"(cr2));
    char rflags_buf[10] = "---------";
    if (info->rflags & (1 << 0))
      rflags_buf[8] = 'C';
    if (info->rflags & (1 << 2))
      rflags_buf[7] = 'P';
    if (info->rflags & (1 << 4))
      rflags_buf[6] = 'A';
    if (info->rflags & (1 << 6))
      rflags_buf[5] = 'Z';
    if (info->rflags & (1 << 7))
      rflags_buf[4] = 'S';
    if (info->rflags & (1 << 8))
      rflags_buf[3] = 'T';
    if (info->rflags & (1 << 9))
      rflags_buf[2] = 'I';
    if (info->rflags & (1 << 10))
      rflags_buf[1] = 'D';
    if (info->rflags & (1 << 11))
      rflags_buf[0] = 'O';
    printf("\nKernel fault %s (cpu=%llu, error=0x%llx)\n"
           "RIP=%016llx CS=%04llx SS=%04llx DPL=%llu\n"
           "RFL=%016llx [%s]\n"
           "RSP=%016llx RBP=%016llx CR2=%08llx\n"
           "RSI=%016llx RDI=%016llx\n"
           "RAX=%016llx RCX=%016llx RDX=%016llx\n"
           "RBX=%016llx R8 =%016llx R9 =%016llx\n"
           "R10=%016llx R11=%016llx R12=%016llx\n"
           "R13=%016llx R14=%016llx R15=%016llx\n",
           f.code, cpuid, error_code, info->rip, info->cs & 0xFFF8,
           info->ss & 0xFFF8, info->cs & 0x7, info->rflags, rflags_buf,
           info->rsp, regs->rbp, cr2, regs->rsi, regs->rdi, regs->rax,
           regs->rcx, regs->rdx, regs->rbx, regs->r8, regs->r9, regs->r10,
           regs->r11, regs->r12, regs->r13, regs->r14, regs->r15);
    for (;;)
      asm volatile("hlt");
  } else if (intr == 0x21) {
    printf("KBD %02xh\n", inportb(0x60));
  } else if (intr != 0x20) {
    printf("INT %02xh\n", intr);
  }
  ACPI::EOI();
  return 0;
}
void Interrupts::init() {
  init_lock.lock();
  if (idt != 0) {
    init_lock.release();
    loadVector();
    return;
  }
  fault = Mutex();
  idt = (PIDT)Memory::alloc(sizeof(IDT), 0x1000);
  idt->rec.limit = sizeof(idt->ints) - 1;
  idt->rec.addr = &idt->ints[0];
  handlers = (int_handler*)Memory::alloc(sizeof(int_handler) * 256, 0x1000);
  void* addr = (void*)&__interrupt_wrap;
  for (int i = 0; i < 256; i++) {
    uintptr_t jmp_from = (uintptr_t)&(handlers[i].reljmp);
    uintptr_t jmp_to = (uintptr_t)addr;
    uintptr_t diff = jmp_to - jmp_from - 5;
    handlers[i].push = 0x68;
    handlers[i].int_num = i;
    handlers[i].reljmp = 0xE9;
    handlers[i].diff = diff;

    uintptr_t hptr = (uintptr_t)(&handlers[i]);

    idt->ints[i].rsvd1 = 0;
    idt->ints[i].rsvd2 = 0;
    idt->ints[i].rsvd3 = 0;

    idt->ints[i].selector = 8;
    idt->ints[i].type = 0xE;
    idt->ints[i].dpl = 0;
    idt->ints[i].ist = 1;
    idt->ints[i].present = true;

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

  init_lock.release();
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
  init_lock.lock();
  if (idt == 0) {
    init_lock.release();
    init();
    return;
  }
  asm volatile("lidtq %0\nsti"::"m"(idt->rec));
  init_lock.release();
}

uint16_t Interrupts::getIRQmask() {
  return inportb(0x21) | (inportb(0xA1) << 8);
}

void Interrupts::addCallback(uint8_t intr, intcb* cb) {
  intcbreg *reg = (intcbreg*)Memory::alloc(sizeof(intcbreg));
  reg->cb = cb;
  reg->next = 0;
  reg->prev = 0;

  uint64_t t = EnterCritical();
  callback_locks[intr].lock();
  intcbreg *last = callbacks[intr];
  if (last == 0) {
    callbacks[intr] = reg;
  } else {
    while (last->next != 0)
      last = last->next;
    reg->prev = last;
    last->next = reg;
  }
  callback_locks[intr].release();
  LeaveCritical(t);
}
