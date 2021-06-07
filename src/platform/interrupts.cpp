//    PhoeniX OS Interrupts subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "interrupts.hpp"
#include "acpi.hpp"
#include "pagetable.hpp"
#include "process.hpp"

struct Interrupts::Handler {
  // 68 04 03 02 01  pushq  ${int_num}
  // e9 46 ec 3f 00  jmp . + {diff}
  uint8_t push[1] = { 0x68 };
  uint32_t int_num;
  uint8_t reljmp[1] = { 0xE9 };
  uint32_t diff;

  constexpr Handler(uint32_t int_num, uint32_t diff): int_num(int_num), diff(diff) {}
} PACKED;

Interrupts::REC64 *Interrupts::idt = nullptr;
GDT *Interrupts::gdt = nullptr;
List<Interrupts::Callback*> *Interrupts::callbacks = nullptr;
Mutex Interrupts::callback_locks[256];
Mutex Interrupts::fault;
Interrupts::Handler* Interrupts::handlers = nullptr;

asm(
    ".global __interrupt_wrap;"
    "__interrupt_wrap:;"

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

    // Move interrupt number to first arg
    "mov 120(%rsp), %rdi;"

    // Set interrupt stack pointer to second arg
    "lea 128(%rsp), %rsi;"

    // Save pagetable address in place of interrupt number
    "mov %cr3, %rax;"
    "mov %rax, 120(%rsp);"

    // Set pointer of pagetable address to third arg
    "lea 120(%rsp), %rdx;"

    // Load kernel pagetable
    "__interrupt_pagetable_mov:"
    "mov $0xF0123456, %rax;"
    "mov %rax, %cr3;"

    // Call actual handler (Interrupts::handle)
    "call _ZN10Interrupts6handleEhmPm;"

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

    // Fix stack for error-code padding
    "test %rax, %rax;"
    "jz 1f;"
    "mov 8(%rsp), %rax; mov %rax, 16(%rsp);"
    "mov 0(%rsp), %rax; mov %rax, 8(%rsp);"
    "add $8, %rsp;"
    "1:"

    // Send EOI to local APIC
    "cmpl $0, 2+_intr_lapic_eoi(%rip);"
    "jz 1f;"
    "_intr_lapic_eoi:"
    "mov $0xF0123456, %rax;"
    "movl $0, (%rax);"
    "1:"

    // Restore caller pagetable
    "mov 8(%rsp), %rax;"
    "mov %rax, %cr3;"

    // Send EOI to PIC
    "mov $0x20, %al;"
    "out %al, $0x20;"

    // Restore last register
    "pop %rax;"

    // Fix stack for interrupt number (actually pagetable address)
    "add $8, %rsp;"

    // Return from interrupt
    "iretq;"

    ".align 16");

struct FAULT {
  char code[5];
  bool has_error_code;
} PACKED;

static const FAULT FAULTS[] = {
  /* 00 */{ "#DE", false },
  /* 01 */{ "#DB", false },
  /* 02 */{ "#NMI", false },
  /* 03 */{ "#BP", false },
  /* 04 */{ "#OF", false },
  /* 05 */{ "#BR", false },
  /* 06 */{ "#UD", false },
  /* 07 */{ "#NM", false },

  /* 08 */{ "#DF", true },
  /* 09 */{ "", false },
  /* 0A */{ "#TS", true },
  /* 0B */{ "#NP", true },
  /* 0C */{ "#SS", true },
  /* 0D */{ "#GP", true },
  /* 0E */{ "#PF", true },
  /* 0F */{ "", false },

  /* 10 */{ "#MF", false },
  /* 11 */{ "#AC", true },
  /* 12 */{ "#MC", false },
  /* 13 */{ "#XM", false },
  /* 14 */{ "#VE", false },
  /* 15 */{ "", false },
  /* 16 */{ "", false },
  /* 17 */{ "", false },

  /* 18 */{ "", false },
  /* 19 */{ "", false },
  /* 1A */{ "", false },
  /* 1B */{ "", false },
  /* 1C */{ "", false },
  /* 1D */{ "", false },
  /* 1E */{ "#SX", true },
  /* 1F */{ "", false }
};

struct int_regs {
  uint64_t r15, r14, r13, r12;
  uint64_t r11, r10, r9, r8;
  uint64_t rdi, rsi, rbp;
  uint64_t rbx, rdx, rcx, rax;
} PACKED;
struct int_info {
  uint64_t rip, cs, rflags, rsp, ss;
} PACKED;

void Interrupts::print(uint8_t num, CallbackRegs *regs, uint32_t code, const Process *process) {
  uint64_t cr2;
  uint64_t base;
  asm volatile("mov %%cr2, %0 ; lea __text_start__(%%rip), %q1":"=r"(cr2),"=r"(base));
  uint32_t cpuid = ACPI::getController()->getCPUID();
  char rflags_buf[10] = "---------";
  if (regs->rflags & (1 << 0))
    rflags_buf[8] = 'C';
  if (regs->rflags & (1 << 2))
    rflags_buf[7] = 'P';
  if (regs->rflags & (1 << 4))
    rflags_buf[6] = 'A';
  if (regs->rflags & (1 << 6))
    rflags_buf[5] = 'Z';
  if (regs->rflags & (1 << 7))
    rflags_buf[4] = 'S';
  if (regs->rflags & (1 << 8))
    rflags_buf[3] = 'T';
  if (regs->rflags & (1 << 9))
    rflags_buf[2] = 'I';
  if (regs->rflags & (1 << 10))
    rflags_buf[1] = 'D';
  if (regs->rflags & (1 << 11))
    rflags_buf[0] = 'O';
  printf("\n%s fault %s (cpu=%u, error=0x%x)\n"
         "BASE=%016lx CS=%04hx SS=%04hx DPL=%hhu\n"
         "IP=%016lx FL=%016lx [%s]\n"
         "SP=%016lx BP=%016lx CR2=%08lx\n"
         "SI=%016lx DI=%016lx\n"
         "A =%016lx C =%016lx D =%016lx B =%016lx\n"
         "8 =%016lx 9 =%016lx 10=%016lx 11=%016lx\n"
         "12=%016lx 13=%016lx 14=%016lx 15=%016lx\n",
         regs->dpl ? "Userspace" : "Kernel", FAULTS[num].code, cpuid, code,
         base, regs->cs, regs->ss, regs->dpl,
         regs->rip, regs->rflags, rflags_buf,
         regs->rsp, regs->rbp, cr2,
         regs->rsi, regs->rdi,
         regs->rax, regs->rcx, regs->rdx, regs->rbx,
         regs->r8, regs->r9, regs->r10, regs->r11,
         regs->r12, regs->r13, regs->r14, regs->r15);

  Process::print_stacktrace(regs->rbp, process);
}

uint64_t __attribute__((sysv_abi)) Interrupts::handle(
    unsigned char intr, uint64_t stack, uint64_t *pagetable) {
  {
    Mutex::Lock lock(fault);
  }
  uint64_t *rsp = reinterpret_cast<uint64_t*>(stack);
  bool has_code = (intr < 0x20) && FAULTS[intr].has_error_code;
  uint32_t error_code = 0;
  int_regs *regs = reinterpret_cast<int_regs*>(rsp - (12 + 3) - 1);
  if (has_code)
    error_code = uint32_t(*(rsp++));
  int_info *info = reinterpret_cast<int_info*>(rsp);
  bool handled = false;

  uint32_t cpuid = ACPI::getController()->getCPUID();

  CallbackRegs cb_regs = {
      cpuid, *pagetable,

      info->rip,
      uint16_t(info->cs & 0xFFF8), info->rflags, info->rsp,
      uint16_t(info->ss & 0xFFF8), uint8_t(info->cs & 7),

      regs->rax, regs->rcx, regs->rdx, regs->rbx,
      regs->rbp, regs->rsi, regs->rdi,
      regs->r8, regs->r9, regs->r10, regs->r11,
      regs->r12, regs->r13, regs->r14, regs->r15
  };

  size_t idx = 0;
  Callback *cb;
  for (;;) {
    {
      Mutex::Lock lock(callback_locks[intr]);
      cb = (callbacks[intr].getCount() > idx) ? callbacks[intr][idx] : nullptr;
      idx++;
    }
    if (cb == nullptr) break;
    handled = cb(intr, error_code, &cb_regs);
    if (handled) break;
  }

  if (handled) {
    *regs = {
      cb_regs.r15, cb_regs.r14, cb_regs.r13, cb_regs.r12,
      cb_regs.r11, cb_regs.r10, cb_regs.r9, cb_regs.r8,
      cb_regs.rdi, cb_regs.rsi, cb_regs.rbp,
      cb_regs.rbx, cb_regs.rdx, cb_regs.rcx, cb_regs.rax,
    };
    *info = {
      cb_regs.rip, uint64_t(cb_regs.cs | cb_regs.dpl),
      cb_regs.rflags | 0x202,
      cb_regs.rsp, uint64_t(cb_regs.ss | cb_regs.dpl)
    };
    *pagetable = cb_regs.cr3;
    return has_code ? 8 : 0;
  }

  if (intr < 0x20) {
    fault.lock();
    print(intr, &cb_regs, error_code);
    for (;;)
      asm volatile("hlt");
  } else if (intr == 0x21) {
    printf("KBD %02xh\n", Port<0x60>::in<uint8_t>());
  } else if (intr != 0x20) {
    printf("INT %02xh\n", intr);
  }
  return 0;
}
void Interrupts::init() {
  uint64_t ncpu = ACPI::getController()->getCPUCount();

  gdt = reinterpret_cast<GDT*>(
      Pagetable::lowalloc((GDT::size(ncpu) + 0xFFF) / 0x1000));
  idt = reinterpret_cast<REC64*>(
      Pagetable::alloc((sizeof(REC64) * 256 + 0xFFF) / 0x1000));
  handlers = reinterpret_cast<Interrupts::Handler*>(
      Pagetable::alloc((sizeof(Handler) * 256 + 0xFFF) / 0x1000));

  gdt->ents[0] = GDT::Entry();
  gdt->ents[1] = GDT::Entry(0, 0xFFFFFFFFFFFFFFFF, 0xA, 0, 1, 1, 0, 1, 0, 1);
  gdt->ents[2] = GDT::Entry(0, 0xFFFFFFFFFFFFFFFF, 0x2, 0, 1, 1, 0, 0, 1, 1);
  gdt->ents[3] = GDT::Entry(0, 0xFFFFFFFFFFFFFFFF, 0x2, 3, 1, 1, 0, 0, 1, 1);
  gdt->ents[4] = GDT::Entry(0, 0xFFFFFFFFFFFFFFFF, 0xA, 3, 1, 1, 0, 1, 0, 1);

  size_t tss_size = sizeof(TSS64_ENT) * ncpu;
  size_t tss_size_pad = klib::__align(tss_size, 0x1000);
  char *tssbuf = reinterpret_cast<char*>(Pagetable::alloc(2 + tss_size_pad / 0x1000));
  char *tsstop = tssbuf + tss_size_pad;
  Memory::fill(tsstop, 0xFF, 0x2000);
  for (uint32_t idx = 0; idx < ncpu; idx++) {
    void *stack = Pagetable::alloc();
    uintptr_t stack_ptr = uintptr_t(stack) + 0x1000;
    char *entbuf = tssbuf + sizeof(TSS64_ENT) * idx;
    TSS64_ENT *ent = reinterpret_cast<TSS64_ENT*>(entbuf);
    ent->ist[0] = stack_ptr;
    ent->iomap_base = uint16_t(tsstop - entbuf);

    gdt->sys_ents[idx] = GDT::SystemEntry(
        uintptr_t(ent), uintptr_t(tsstop - entbuf) + 0x2000 - 1,
        0x9, 0, 0, 1, 0, 1, 0, 0);
  }

  DTREG gdtreg = { uint16_t(GDT::size(ncpu) -1), &gdt->ents[0] };

  asm volatile(
      "mov %%rsp, %%rcx;"
      "pushq $16;"
      "pushq %%rcx;"
      "pushfq;"
      "pushq $8;"
      "lea 1f(%%rip), %%rcx;"
      "pushq %%rcx;"
      "lgdtq %0;"
      "iretq;"
      "1:"
      "mov %%ss, %%ax;"
      "mov %%ax, %%ds;"
      "mov %%ax, %%es;"
      "mov %%ax, %%gs;"
      ::"m"(gdtreg):"ax", "rcx");

  uintptr_t addr;
  char *lapic_eoi =
      reinterpret_cast<char*>(ACPI::getController()->getLapicAddr());
  if (lapic_eoi) lapic_eoi += ACPI::LAPIC_EOI;
  asm volatile("lea __interrupt_wrap(%%rip), %q0":"=r"(addr));
  asm volatile(
      "mov %%eax, 2+_intr_lapic_eoi(%%rip);"
      "mov %%cr3, %%rax;"
      "mov %%eax, 2+__interrupt_pagetable_mov(%%rip)"
      ::"a"(lapic_eoi)
      );
  for (uint32_t i = 0; i < 256; i++) {
    uintptr_t jmp_from = uintptr_t(&(handlers[i].reljmp));
    uintptr_t diff = addr - jmp_from - 5;
    handlers[i] = Handler(i, uint32_t(diff));

    uintptr_t hptr = uintptr_t(&handlers[i]);
    idt[i] = REC64(hptr, 8, 1, 0xE, 0, true);
  }
  callbacks = new List<Callback*>[256]();

  Port<0x20>::out<uint8_t>(0x11);
  Port<0xA0>::out<uint8_t>(0x11);
  Port<0x21>::out<uint8_t>(0x20);
  Port<0xA1>::out<uint8_t>(0x28);
  Port<0x21>::out<uint8_t>(0x04);
  Port<0xA1>::out<uint8_t>(0x02);
  Port<0x21>::out<uint8_t>(0x01);
  Port<0xA1>::out<uint8_t>(0x01);

  loadVector();

  if (!(ACPI::getController())->initAPIC()) {
    Port<0x43>::out<uint8_t>(0x34);
    static const uint16_t rld = 0x000F;
    Port<0x40>::out<uint8_t>(rld & 0xFF);
    Port<0x40>::out<uint8_t>((rld >> 8) & 0xFF);
  }
  maskIRQ(0);
}

void Interrupts::maskIRQ(uint16_t mask) {
  Port<0x21>::out<uint8_t>(mask & 0xFF);
  Port<0xA1>::out<uint8_t>((mask >> 8) & 0xFF);
}

void Interrupts::loadVector() {
  if (idt == nullptr) {
    init();
    return;
  }
  DTREG idtreg = { sizeof(REC64) * 256 -1, &idt[0] };
  uint32_t cpuid = ACPI::getController()->getCPUID();
  uint16_t tr = uint16_t(5 * sizeof(GDT::Entry) + cpuid * sizeof(GDT::SystemEntry));
  asm volatile(
      "lidtq %0;"
      "ltr %w1;"
      "sti"::"m"(idtreg), "r"(tr));
}

uint16_t Interrupts::getIRQmask() {
  return uint16_t(
      (uint16_t(Port<0x21>::in<uint8_t>())) |
      (uint16_t(Port<0xA1>::in<uint8_t>()) << 8));
}

void Interrupts::addCallback(uint8_t intr, Callback* cb) {
  Mutex::CriticalLock lock(callback_locks[intr]);
  callbacks[intr].add(cb);
}
