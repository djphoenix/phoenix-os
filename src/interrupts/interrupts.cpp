//    PhoeniX OS Interrupts subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "interrupts.hpp"
#include "acpi.hpp"
#include "pagetable.hpp"
#include "list.hpp"

struct Interrupts::Handler {
  // 48 83 ec 00     sub    ${subrsp}, rsp
  // 6A 01           pushq  ${int_num}
  // e9 46 ec 3f 00  jmp    . + {diff}
  uint8_t sub_rsp[3] = { 0x48, 0x83, 0xEC };
  uint8_t subrsp;
  uint8_t push[1] = { 0x6A };
  uint8_t int_num;
  uint8_t reljmp[1] = { 0xE9 };
  uint32_t diff;

  constexpr Handler(uint8_t int_num, uint8_t subrsp, uint32_t diff): subrsp(subrsp), int_num(int_num), diff(diff) {}
} PACKED;

Interrupts::REC64 *Interrupts::idt = nullptr;
GDT *Interrupts::gdt = nullptr;
List<Interrupts::Callback*> *Interrupts::callbacks = nullptr;
Mutex Interrupts::callback_locks[256];
Mutex Interrupts::fault;
Interrupts::Handler* Interrupts::handlers = nullptr;
__attribute__((used))
uintptr_t Interrupts::eoi_vector = 0;

extern "C" void __attribute__((naked)) __attribute__((used)) __interrupt_wrap() {
  asm(
    // Save registers
    "push %r15;"
    "push %r14;"
    "push %r13;"
    "push %r12;"
    "push %r11;"
    "push %r10;"
    "push %r9;"
    "push %r8;"
    "push %rdi;"
    "push %rsi;"
    "push %rbp;"
    "push %rbx;"
    "push %rdx;"
    "push %rcx;"
    "push %rax;"

    // Move interrupt number to first arg
    // Save pagetable address in place of interrupt number
    "mov %cr3, %rdi;"
    "xchg %rdi, 0x78(%rsp);"

    // Set interrupt stack pointer to second arg
    "lea 0x80(%rsp), %rsi;"

    // Set pointer of pagetable address to third arg
    "lea 0x78(%rsp), %rdx;"

    // Load kernel pagetable
    "__interrupt_pagetable_mov:"
    "movabsq $0, %rax;"
    "mov %rax, %cr3;"

    "sub $0x200, %rsp;"
    "fxsave (%rsp);"
    // Set pointer to SSE regietsrs to 4th arg
    "mov %rsp, %rcx;"

    // Call actual handler (Interrupts::handle)
    "lea 0(%rsi), %rbp;"
    "call _ZN10Interrupts6handleEhmPmPN6Thread3SSEE;"

    "fxrstor (%rsp);"

    // Restore registers
    "add $0x200, %rsp;"

    "pop %r15;"
    "pop %rcx;"
    "pop %rdx;"
    "pop %rbx;"
    "pop %rbp;"
    "pop %rsi;"
    "pop %rdi;"
    "pop %r8;"
    "pop %r9;"
    "pop %r10;"
    "pop %r11;"
    "pop %r12;"
    "pop %r13;"
    "pop %r14;"

    // Send EOI to local APIC
    "mov _ZN10Interrupts10eoi_vectorE(%rip), %rax;"
    "test %rax, %rax; jz 1f;"
    "movl $0, (%rax); 1:"

    // Restore caller pagetable
    "mov 0x8(%rsp), %rax;"
    "mov %rax, %cr3;"

    // Send EOI to PIC
    "mov $0x20, %al;"
    "out %al, $0x20;"

    // Restore last register
    "pop %rax;"
    "xchg %rax, %r15;"

    // Fix stack for interrupt number (actually pagetable address)
    "add $0x10, %rsp;"

    // Return from interrupt
    "iretq;"
  );
}

const Interrupts::FAULT Interrupts::faults[] = {
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

struct int_info {
  uint64_t error_code;
  uint64_t rip, cs, rflags, rsp, ss;
} PACKED;

void __attribute__((sysv_abi)) __attribute__((used)) Interrupts::handle(
    unsigned char intr, uint64_t stack, uint64_t *pagetable, Thread::SSE *sse) {
  {
    Mutex::Lock lock(fault);
  }
  uint64_t *const rsp = reinterpret_cast<uint64_t*>(stack);
  Thread::Regs *const regs = reinterpret_cast<Thread::Regs*>(rsp - (12 + 3) - 1);
  int_info *const info = reinterpret_cast<int_info*>(rsp);
  bool handled = false;

  uint32_t cpuid = ACPI::getController()->getCPUID();

  CallbackRegs::Info cb_info = {
    *pagetable,
    info->rip,
    info->rflags,
    info->rsp,
    uint16_t(info->cs & 0xFFF8), 
    uint16_t(info->ss & 0xFFF8),
    uint8_t(info->cs & 7),
  };
  CallbackRegs cb_regs = {
    cpuid,
    &cb_info,
    regs,
    sse,
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
    handled = cb(intr, uint32_t(info->error_code), &cb_regs);
    if (handled) break;
  }

  if (handled) {
    *info = {
      info->error_code,
      cb_info.rip, uint64_t(cb_info.cs | cb_info.dpl),
      cb_info.rflags | 0x202,
      cb_info.rsp, uint64_t(cb_info.ss | cb_info.dpl)
    };
    *pagetable = cb_info.cr3;
    return;
  }

  char printbuf[32];
  if (intr < 0x20) {
    fault.lock();
    for (;;) asm volatile("hlt");
  } else if (intr == 0x21) {
    snprintf(printbuf, sizeof(printbuf), "KBD %02xh\n", Port<0x60>::in8());
    klib::puts(printbuf);
  } else if (intr != 0x20) {
    snprintf(printbuf, sizeof(printbuf), "INT %02xh\n", intr);
    klib::puts(printbuf);
  }
}
void Interrupts::init() {
  uint64_t ncpu = ACPI::getController()->getCPUCount();

  gdt = reinterpret_cast<GDT*>(
      Pagetable::lowalloc((GDT::size(ncpu) + 0xFFF) / 0x1000, Pagetable::MemoryType::DATA_RW));
  idt = reinterpret_cast<REC64*>(
      Pagetable::alloc((sizeof(REC64) * 256 + 0xFFF) / 0x1000, Pagetable::MemoryType::DATA_RW));

  gdt->ents[0] = GDT::Entry();
  gdt->ents[1] = GDT::Entry(0, 0xFFFFFFFFFFFFFFFF, 0xA, 0, 1, 1, 0, 1, 0, 1);
  gdt->ents[2] = GDT::Entry(0, 0xFFFFFFFFFFFFFFFF, 0x2, 0, 1, 1, 0, 0, 1, 1);
  gdt->ents[3] = GDT::Entry(0, 0xFFFFFFFFFFFFFFFF, 0x2, 3, 1, 1, 0, 0, 1, 1);
  gdt->ents[4] = GDT::Entry(0, 0xFFFFFFFFFFFFFFFF, 0xA, 3, 1, 1, 0, 1, 0, 1);

  size_t tss_size = sizeof(TSS64_ENT) * ncpu;
  size_t tss_size_pad = klib::__align(tss_size, 0x1000);
  uint8_t *tssbuf = reinterpret_cast<uint8_t*>(Pagetable::alloc(2 + tss_size_pad / 0x1000, Pagetable::MemoryType::DATA_RW));
  uint8_t *tsstop = tssbuf + tss_size_pad;
  Memory::fill(tsstop, 0xFF, 0x2000);
  for (uint32_t idx = 0; idx < ncpu; idx++) {
    void *stack = Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW);
    uintptr_t stack_ptr = uintptr_t(stack) + 0x1000;
    uint8_t *entbuf = tssbuf + sizeof(TSS64_ENT) * idx;
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
      "lea 1f(%%rip), %%rax;"
      "pushq $16;"
      "pushq %%rcx;"
      "pushfq;"
      "pushq $8;"
      "pushq %%rax;"
      "lgdtq (%0);"
      "iretq;"
      "1:"
      "mov %%ss, %%ax;"
      "mov %%ax, %%ds;"
      "mov %%ax, %%es;"
      "mov %%ax, %%gs;"
      ::"r"(&gdtreg):"rax", "rcx");

  uintptr_t addr;
  uint8_t *lapic_eoi =
      reinterpret_cast<uint8_t*>(ACPI::getController()->getLapicAddr());
  if (lapic_eoi) lapic_eoi += ACPI::LAPIC_EOI;
  eoi_vector = uintptr_t(lapic_eoi);
  asm volatile("lea __interrupt_wrap(%%rip), %q0":"=r"(addr));
  Pagetable::map(reinterpret_cast<void*>(addr), Pagetable::MemoryType::CODE_RW);
  asm volatile(
    "mov %%cr3, %%rax;"
    "mov %%eax, 2+__interrupt_pagetable_mov(%%rip)"
    :::"rax"
  );
  Pagetable::map(reinterpret_cast<void*>(addr), Pagetable::MemoryType::CODE_RX);

  handlers = reinterpret_cast<Interrupts::Handler*>(
      Pagetable::alloc((sizeof(Handler) * 256 + 0xFFF) / 0x1000, Pagetable::MemoryType::CODE_RW));
  for (uint32_t i = 0; i < 256; i++) {
    uintptr_t jmp_from = uintptr_t(&(handlers[i].reljmp));
    uintptr_t diff = addr - jmp_from - 5;
    handlers[i] = Handler(uint8_t(i), uint8_t((i < 0x20) ? (faults[i].has_error_code ? 0 : 8) : 8), uint32_t(diff));

    uintptr_t hptr = uintptr_t(&handlers[i]);
    idt[i] = REC64(hptr, 8, 1, 0xE, 0, true);
  }
  Pagetable::map(handlers, handlers + 256, Pagetable::MemoryType::CODE_RX);
  callbacks = new List<Callback*>[256]();

  Port<0x20>::out8(0x11);
  Port<0xA0>::out8(0x11);
  Port<0x21>::out8(0x20);
  Port<0xA1>::out8(0x28);
  Port<0x21>::out8(0x04);
  Port<0xA1>::out8(0x02);
  Port<0x21>::out8(0x01);
  Port<0xA1>::out8(0x01);

  loadVector();

  if (!(ACPI::getController())->initAPIC()) {
    Port<0x43>::out8(0x34);
    static const uint16_t rld = 0x000F;
    Port<0x40>::out8(rld & 0xFF);
    Port<0x40>::out8((rld >> 8) & 0xFF);
  }
  maskIRQ(0);
}

void Interrupts::maskIRQ(uint16_t mask) {
  Port<0x21>::out8(mask & 0xFFu);
  Port<0xA1>::out8(uint8_t(mask >> 8));
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
      (uint16_t(Port<0x21>::in8())) |
      (uint16_t(Port<0xA1>::in8()) << 8));
}

void Interrupts::addCallback(uint8_t intr, Callback* cb) {
  Mutex::CriticalLock lock(callback_locks[intr]);
  callbacks[intr].add(cb);
}
