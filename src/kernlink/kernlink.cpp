#include "kernlink.hpp"

#include "acpi.hpp"
#include "syscall.hpp"
#include "interrupts.hpp"
#include "process.hpp"

#include "kernlib/std.hpp"
#include "kernlib/mem.hpp"
#include "rand.hpp"

struct ProcessSymbol {
  uintptr_t ptr;
  char* name;
};

void KernelLinker::addSymbol(const char *name, uintptr_t ptr) {
  symbols.insert() = { ptr, klib::strdup(name) };
}
uintptr_t KernelLinker::getSymbolByName(const char* name) const {
  for (size_t i = 0; i < symbols.getCount(); i++) {
    if (klib::strcmp(symbols[i].name, name) == 0)
      return symbols[i].ptr;
  }
  return 0;
}
struct SyscallEntry {
  const uint8_t _pre[2] = {
    // movabsq ..., %rax
    0x48, 0xb8,
  };
  const uint64_t syscall_id;
  const uint8_t _post[3] = {
    // syscallq; ret
    0x0f, 0x05, 0xc3,
  };

  explicit constexpr SyscallEntry(uint64_t idx) : syscall_id(idx) {}
} __attribute__((packed));
uintptr_t KernelLinker::linkLibrary(const char* funcname) {
  if (klib::strcmp(funcname, "kptr_acpi_rsdp") == 0) {
    return uintptr_t(ACPI::getController()->getRSDPAddr());
  }

  uintptr_t ptr = getSymbolByName(funcname);
  if (ptr != 0) return ptr;

  if (klib::strcmp(funcname, "__stack_chk_guard") == 0) {
    uint64_t rnd = RAND::get<uint64_t>();
    uintptr_t page = process->addSection(0, SectionTypeData, sizeof(rnd));
    process->writeData(page, &rnd, sizeof(rnd));
    addSymbol(funcname, page);
    return page;
  }
  if (klib::strcmp(funcname, "__stack_chk_fail") == 0) {
    static const constexpr size_t countInPage = 0x1000 / sizeof(SyscallEntry);
    if (_syscallPage == 0 || _syscallNum == countInPage) {
      _syscallPage = process->addSection(0, SectionTypeCode, sizeof(SyscallEntry) * countInPage);
      _syscallNum = 0;
    }
    uint8_t call[sizeof(SyscallEntry)] = { 0x0B, 0x0F, };
    ptr = _syscallPage + (_syscallNum++) * sizeof(SyscallEntry);
    process->writeData(ptr, call, sizeof(call));
    addSymbol(funcname, ptr);
  }

  uint64_t syscall_id;
  if ((syscall_id = Syscall::callByName(funcname)) != 0) {
    static const constexpr size_t countInPage = 0x1000 / sizeof(SyscallEntry);
    if (!_syscallPage) {
      uintptr_t sc_wrapper = uintptr_t(Syscall::wrapper) & (~0xFFFllu);
      process->addPage(sc_wrapper, reinterpret_cast<void*>(sc_wrapper), Pagetable::MemoryType::CODE_RX);
      process->addPage(sc_wrapper + 0x1000, reinterpret_cast<void*>(sc_wrapper + 0x1000), Pagetable::MemoryType::CODE_RX);
    }
    if (_syscallPage == 0 || _syscallNum == countInPage) {
      _syscallPage = process->addSection(0, SectionTypeCode, sizeof(SyscallEntry) * countInPage);
      _syscallNum = 0;
    }
    SyscallEntry call(syscall_id);
    ptr = _syscallPage + (_syscallNum++) * sizeof(SyscallEntry);
    process->writeData(ptr, &call, sizeof(call));
    addSymbol(funcname, ptr);
  }

  return ptr;
}

void KernelLinker::prepareToStart() {
  DTREG gdt, idt;
  asm volatile("sgdtq %0; sidtq %1":"=m"(gdt), "=m"(idt));

  static const uintptr_t KB4 = ~0xFFFllu;
  for (uintptr_t addr = uintptr_t(gdt.addr) & KB4;
      addr < (uintptr_t(gdt.addr) + gdt.limit); addr += 0x1000) {
    process->addPage(addr, reinterpret_cast<void*>(addr), Pagetable::MemoryType::DATA_RW);
  }
  for (uintptr_t addr = uintptr_t(idt.addr) & KB4;
      addr < (uintptr_t(idt.addr) + idt.limit); addr += 0x1000) {
    process->addPage(addr, reinterpret_cast<void*>(addr), Pagetable::MemoryType::DATA_RO);
  }
  Interrupts::REC64 *recs = static_cast<Interrupts::REC64*>(idt.addr);
  uintptr_t page = 0;
  for (uint16_t i = 0; i < 0x100; i++) {
    uintptr_t handler = (uintptr_t(recs[i].offset_low)
        | (uintptr_t(recs[i].offset_middle) << 16)
        | (uintptr_t(recs[i].offset_high) << 32));
    if (page != (handler & KB4)) {
      page = handler & KB4;
      process->addPage(page, reinterpret_cast<void*>(page), Pagetable::MemoryType::CODE_RX);
    }
  }
  uintptr_t handler = uintptr_t(Interrupts::wrapper) & KB4;
  process->addPage(handler, reinterpret_cast<void*>(handler), Pagetable::MemoryType::CODE_RX);
  process->addPage(handler + 0x1000, reinterpret_cast<void*>(handler + 0x1000), Pagetable::MemoryType::CODE_RX);
  GDT::Entry *gdt_ent = reinterpret_cast<GDT::Entry*>(uintptr_t(gdt.addr) + 8 * 3);
  GDT::Entry *gdt_top = reinterpret_cast<GDT::Entry*>(uintptr_t(gdt.addr) + gdt.limit);

  if (!process->iomap[0]) {
    process->iomap[0] = Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW);
    Memory::fill(process->iomap[0], 0xFF, 0x1000);
  }
  if (!process->iomap[1]) {
    process->iomap[1] = Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW);
    Memory::fill(process->iomap[1], 0xFF, 0x1000);
  }

  while (gdt_ent < gdt_top) {
    uintptr_t base = gdt_ent->getBase();
    if ((gdt_ent->type != 0x9) && (gdt_ent->type != 0xB)) {
      gdt_ent++;
      continue;
    }

    GDT::SystemEntry *sysent = reinterpret_cast<GDT::SystemEntry*>(gdt_ent);
    base = sysent->getBase();

    uintptr_t page = base & KB4;
    process->addPage(page, reinterpret_cast<void*>(page), Pagetable::MemoryType::DATA_RO);
    process->addPage(page + 0x1000, process->iomap[0], Pagetable::MemoryType::DATA_RO);
    process->addPage(page + 0x2000, process->iomap[1], Pagetable::MemoryType::DATA_RO);

    TSS64_ENT *tss = reinterpret_cast<TSS64_ENT*>(base);
    uintptr_t stack = tss->ist[0];
    page = stack - 0x1000;
    process->addPage(page, reinterpret_cast<void*>(page), Pagetable::MemoryType::DATA_RW);
    gdt_ent = reinterpret_cast<GDT::Entry*>(sysent + 1);
  }
}

KernelLinker::~KernelLinker() {
  for (size_t i = 0; i < symbols.getCount(); i++) {
    delete[] symbols[i].name;
  }
}
