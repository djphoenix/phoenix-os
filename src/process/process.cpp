//    PhoeniX OS Process subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "process.hpp"

#include "processmanager.hpp"
#include "syscall.hpp"
#include "acpi.hpp"

using PTE = Pagetable::Entry;

Process::Process() :
    id(uint64_t(-1)), entry(0),
    _aslrCode(RAND::get<uintptr_t>(0x80000000llu, 0x100000000llu) << 12),
    _aslrStack(RAND::get<uintptr_t>(0x40000000llu, 0x80000000llu) << 12),
    _syscallPage(0), _syscallNum(0),
    name(nullptr),
    pagetable(nullptr) {
  iomap[0] = iomap[1] = nullptr;
}

Process::~Process() {
  void *rsp; asm volatile("mov %%rsp, %q0; and $~0xFFF, %q0":"=r"(rsp));
  if (pagetable != nullptr) {
    PTE addr;
    for (uintptr_t ptx = 0; ptx < 512; ptx++) {
      addr = pagetable[ptx];
      if (!addr.present)
        continue;
      PTE *ppde = addr.getPTE();
      for (uintptr_t pdx = 0; pdx < 512; pdx++) {
        addr = ppde[pdx];
        if (!addr.present)
          continue;
        PTE *pppde = addr.getPTE();
        for (uintptr_t pdpx = 0; pdpx < 512; pdpx++) {
          addr = pppde[pdpx];
          if (!addr.present)
            continue;
          PTE *ppml4e = addr.getPTE();
          for (uintptr_t pml4x = 0; pml4x < 512; pml4x++) {
            addr = ppml4e[pml4x];
            if (!addr.present)
              continue;
            void *page = addr.getPtr();
            uintptr_t ptaddr = ((ptx << (12 + 9 + 9 + 9))
                | (pdx << (12 + 9 + 9))
                | (pdpx << (12 + 9)) | (pml4x << (12)));
            if (page == rsp) continue;
            if (uintptr_t(page) == ptaddr) continue;
            Pagetable::free(page);
          }
          Pagetable::free(ppml4e);
        }
        Pagetable::free(pppde);
      }
      Pagetable::free(ppde);
    }
    Pagetable::free(pagetable);
  }
  for (size_t i = 0; i < symbols.getCount(); i++) {
    delete[] symbols[i].name;
  }
  for (size_t i = 0; i < threads.getCount(); i++) {
    ProcessManager::getManager()->dequeueThread(threads[i]);
    delete threads[i];
  }
  delete name;
}

PTE* Process::addPage(uintptr_t vaddr, void* paddr, Pagetable::MemoryType type) {
  uint16_t ptx = (vaddr >> (12 + 9 + 9 + 9)) & 0x1FF;
  uint16_t pdx = (vaddr >> (12 + 9 + 9)) & 0x1FF;
  uint16_t pdpx = (vaddr >> (12 + 9)) & 0x1FF;
  uint16_t pml4x = (vaddr >> (12)) & 0x1FF;
  if (pagetable == nullptr) {
    pagetable = static_cast<PTE*>(Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW));
    addPage(uintptr_t(pagetable), pagetable, Pagetable::MemoryType::USER_TABLE);
  }
  if (!pagetable[ptx].present) {
    pagetable[ptx] = PTE(Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW), Pagetable::MemoryType::USER_TABLE);
    addPage(pagetable[ptx].getUintPtr(), pagetable[ptx].getPtr(), Pagetable::MemoryType::USER_TABLE);
  }
  PTE *pde = pagetable[ptx].getPTE();
  if (!pde[pdx].present) {
    pde[pdx] = PTE(Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW), Pagetable::MemoryType::USER_TABLE);
    addPage(pde[pdx].getUintPtr(), pde[pdx].getPtr(), Pagetable::MemoryType::USER_TABLE);
  }
  PTE *pdpe = pde[pdx].getPTE();
  if (!pdpe[pdpx].present) {
    pdpe[pdpx] = PTE(Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW), Pagetable::MemoryType::USER_TABLE);
    addPage(pdpe[pdpx].getUintPtr(), pdpe[pdpx].getPtr(), Pagetable::MemoryType::USER_TABLE);
  }
  PTE *pml4e = pdpe[pdpx].getPTE();
  pml4e[pml4x] = PTE(paddr, type);
  return &pml4e[pml4x];
}

uintptr_t Process::addSection(uintptr_t offset, SectionType type, size_t size) {
  if (size == 0)
    return 0;
  size_t pages = (size >> 12) + 1;
  uintptr_t vaddr;
  if (type != SectionTypeStack) {
    vaddr = _aslrCode + offset;
  } else {
    vaddr = _aslrStack;
  }

  for (uintptr_t caddr = vaddr; caddr < vaddr + size; caddr += 0x1000) {
    if (getPhysicalAddress(vaddr) != nullptr) {
      vaddr += 0x1000;
      caddr = vaddr - 0x1000;
    }
  }
  uintptr_t addr = vaddr;
  while (pages-- > 0) {
    Pagetable::MemoryType ptype;
    switch (type) {
      case SectionTypeCode: ptype = Pagetable::MemoryType::USER_CODE_RX; break;
      case SectionTypeROData: ptype = Pagetable::MemoryType::USER_DATA_RO; break;

      case SectionTypeData:
      case SectionTypeBSS:
      case SectionTypeStack:
        ptype = Pagetable::MemoryType::USER_DATA_RW;
        break;
    }
    addPage(vaddr, Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW), ptype);
    vaddr += 0x1000;
  }
  return addr;
}
void Process::addSymbol(const char *name, uintptr_t ptr) {
  symbols.insert() = { ptr, klib::strdup(name) };
}
void Process::setEntryAddress(uintptr_t ptr) {
  entry = ptr;
}
uintptr_t Process::getSymbolByName(const char* name) const {
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
} PACKED;
uintptr_t Process::linkLibrary(const char* funcname) {
  if (klib::strcmp(funcname, "kptr_acpi_rsdp") == 0) {
    return uintptr_t(ACPI::getController()->getRSDPAddr());
  }

  uintptr_t ptr = getSymbolByName(funcname);
  if (ptr != 0) return ptr;

  if (klib::strcmp(funcname, "__stack_chk_guard") == 0) {
    uint64_t rnd = RAND::get<uint64_t>();
    uintptr_t page = addSection(0, SectionTypeData, sizeof(rnd));
    writeData(page, &rnd, sizeof(rnd));
    addSymbol(funcname, page);
    return page;
  }
  if (klib::strcmp(funcname, "__stack_chk_fail") == 0) {
    static const constexpr size_t countInPage = 0x1000 / sizeof(SyscallEntry);
    if (_syscallPage == 0 || _syscallNum == countInPage) {
      _syscallPage = addSection(0, SectionTypeCode, sizeof(SyscallEntry) * countInPage);
      _syscallNum = 0;
    }
    uint8_t call[sizeof(SyscallEntry)] = { 0x0B, 0x0F, };
    ptr = _syscallPage + (_syscallNum++) * sizeof(SyscallEntry);
    writeData(ptr, call, sizeof(call));
    addSymbol(funcname, ptr);
  }

  uint64_t syscall_id;
  if ((syscall_id = Syscall::callByName(funcname)) != 0) {
    static const constexpr size_t countInPage = 0x1000 / sizeof(SyscallEntry);
    if (_syscallPage == 0 || _syscallNum == countInPage) {
      _syscallPage = addSection(0, SectionTypeCode, sizeof(SyscallEntry) * countInPage);
      _syscallNum = 0;
    }
    SyscallEntry call(syscall_id);
    ptr = _syscallPage + (_syscallNum++) * sizeof(SyscallEntry);
    writeData(ptr, &call, sizeof(call));
    addSymbol(funcname, ptr);
  }

  return ptr;
}
void Process::writeData(uintptr_t address, const void* src, size_t size) {
  const uint8_t *ptr = static_cast<const uint8_t*>(src);
  while (size > 0) {
    void *dest = getPhysicalAddress(address);
    size_t limit = 0x1000 - (uintptr_t(dest) & 0xFFF);
    size_t count = klib::min(size, limit);
    Memory::copy(dest, ptr, count);
    size -= count;
    ptr += count;
    address += count;
  }
}
void Process::readData(void* dst, uintptr_t address, size_t size) const {
  uint8_t *ptr = static_cast<uint8_t*>(dst);
  while (size > 0) {
    void *src = getPhysicalAddress(address);
    size_t limit = 0x1000 - (uintptr_t(src) & 0xFFF);
    size_t count = klib::min(size, limit);
    Memory::copy(ptr, src, count);
    size -= count;
    ptr += count;
    address += count;
  }
}
char *Process::readString(uintptr_t address) const {
  size_t length = 0;
  const uint8_t *src = static_cast<const uint8_t*>(getPhysicalAddress(address));
  size_t limit = 0x1000 - (uintptr_t(src) & 0xFFF);
  while (limit-- && src[length] != 0) {
    length++;
    if (limit == 0) {
      src = static_cast<const uint8_t*>(getPhysicalAddress(address + length));
      limit += 0x1000;
    }
  }
  if (length == 0)
    return nullptr;
  char *buf = new char[length + 1]();
  readData(buf, address, length + 1);
  return buf;
}
void *Process::getPhysicalAddress(uintptr_t ptr) const {
  if (pagetable == nullptr)
    return nullptr;
  uintptr_t off = ptr & 0xFFF;
  PTE *addr = PTE::find(ptr, pagetable);
  if (!addr || !addr->present) return nullptr;
  return reinterpret_cast<void*>(addr->getUintPtr() + off);
}
void Process::addThread(Thread *thread, bool suspended) {
  if (thread->stack_top == 0) {
    thread->stack_top = addSection(0, SectionTypeStack, 0x7FFF) + 0x8000 - 8;
    thread->rsp = thread->stack_top;
  }
  thread->suspend_ticks = suspended ? uint64_t(-1) : 0;

  threads.add(thread);
  ProcessManager::getManager()->queueThread(this, thread);
}
void Process::allowIOPorts(uint16_t min, uint16_t max) {
  for (uint16_t i = 0; i <= (max - min); i++) {
    uint16_t port = min + i;
    size_t space = port / 8 / 0x1000;
    size_t byte = (port / 8) & 0xFFF;
    size_t bit = port % 8;
    uint8_t *map;
    if (!(map = reinterpret_cast<uint8_t*>(iomap[space]))) {
      map = reinterpret_cast<uint8_t*>(iomap[space] = Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW));
      Memory::fill(map, 0xFF, 0x1000);
    }
    map[byte] &= ~(1 << bit);
  }
}
void Process::startup() {
  DTREG gdt = { 0, nullptr };
  DTREG idt = { 0, nullptr };
  asm volatile("sgdtq %0; sidtq %1":"=m"(gdt), "=m"(idt));

  static const uintptr_t KB4 = 0xFFFFFFFFFFFFF000;
  for (uintptr_t addr = uintptr_t(gdt.addr) & KB4;
      addr < (uintptr_t(gdt.addr) + gdt.limit); addr += 0x1000) {
    addPage(addr, reinterpret_cast<void*>(addr), Pagetable::MemoryType::DATA_RW);
  }
  for (uintptr_t addr = uintptr_t(idt.addr) & KB4;
      addr < (uintptr_t(idt.addr) + idt.limit); addr += 0x1000) {
    addPage(addr, reinterpret_cast<void*>(addr), Pagetable::MemoryType::DATA_RO);
  }
  Interrupts::REC64 *recs = static_cast<Interrupts::REC64*>(idt.addr);
  uintptr_t page = 0;
  for (uint16_t i = 0; i < 0x100; i++) {
    uintptr_t handler = (uintptr_t(recs[i].offset_low)
        | (uintptr_t(recs[i].offset_middle) << 16)
        | (uintptr_t(recs[i].offset_high) << 32));
    if (page != (handler & KB4)) {
      page = handler & KB4;
      addPage(page, reinterpret_cast<void*>(page), Pagetable::MemoryType::CODE_RX);
    }
  }
  uintptr_t handler, sc_wrapper = uintptr_t(Syscall::wrapper);
  asm volatile("lea __interrupt_wrap(%%rip), %q0":"=r"(handler));
  handler &= KB4;
  sc_wrapper &= KB4;
  addPage(handler, reinterpret_cast<void*>(handler), Pagetable::MemoryType::CODE_RX);
  addPage(handler + 0x1000, reinterpret_cast<void*>(handler + 0x1000), Pagetable::MemoryType::CODE_RX);
  addPage(sc_wrapper, reinterpret_cast<void*>(sc_wrapper), Pagetable::MemoryType::CODE_RX);
  addPage(sc_wrapper + 0x1000, reinterpret_cast<void*>(sc_wrapper + 0x1000), Pagetable::MemoryType::CODE_RX);
  GDT::Entry *gdt_ent = reinterpret_cast<GDT::Entry*>(uintptr_t(gdt.addr) + 8 * 3);
  GDT::Entry *gdt_top = reinterpret_cast<GDT::Entry*>(uintptr_t(gdt.addr) + gdt.limit);

  if (!iomap[0]) {
    iomap[0] = Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW);
    Memory::fill(iomap[0], 0xFF, 0x1000);
  }
  if (!iomap[1]) {
    iomap[1] = Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW);
    Memory::fill(iomap[1], 0xFF, 0x1000);
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
    addPage(page, reinterpret_cast<void*>(page), Pagetable::MemoryType::DATA_RO);
    addPage(page + 0x1000, iomap[0], Pagetable::MemoryType::DATA_RO);
    addPage(page + 0x2000, iomap[1], Pagetable::MemoryType::DATA_RO);

    TSS64_ENT *tss = reinterpret_cast<TSS64_ENT*>(base);
    uintptr_t stack = tss->ist[0];
    page = stack - 0x1000;
    addPage(page, reinterpret_cast<void*>(page), Pagetable::MemoryType::DATA_RW);
    gdt_ent = reinterpret_cast<GDT::Entry*>(sysent + 1);
  }

  Thread *thread = new Thread();
  thread->rip = entry;
  thread->sse.sse[3] = 0x0000ffff00001F80llu;
  thread->rflags = 0;
  id = (ProcessManager::getManager())->registerProcess(this);
  addThread(thread, false);
}

__attribute__((used))
void Process::exit(int code) {
  (void)code;  // TODO: handle
  for (size_t i = 0; i < threads.getCount(); i++) {
    ProcessManager::getManager()->dequeueThread(threads[i]);
  }
  ProcessManager::getManager()->exitProcess(this, code);
  delete this;
}

const char *Process::getName() const { return name; }
void Process::setName(const char *newname) {
  delete name;
  name = newname ? klib::strdup(newname) : nullptr;
}

void Process::print_stacktrace(uintptr_t base, const Process *process) {
  struct stackframe {
    struct stackframe* rbp;
    uintptr_t rip;
  } __attribute__((packed));

  printf("STACK TRACE:");
  struct stackframe tmpframe;
  const struct stackframe *frame;
  if (base) {
    frame = reinterpret_cast<struct stackframe*>(base);
  } else {
    asm volatile("mov %%rbp, %q0":"=r"(frame)::);
  }
  size_t lim = 10;
  while (lim-- && frame != nullptr) {
    if (process) {
      process->readData(&tmpframe.rbp, uintptr_t(frame), sizeof(uintptr_t));
      if (tmpframe.rbp) {
        process->readData(&tmpframe.rip, uintptr_t(frame)+sizeof(uintptr_t), sizeof(uintptr_t));
      } else {
        break;
      }
      frame = &tmpframe;
    }
    printf(" [%p]:%p", reinterpret_cast<void*>(frame->rbp), reinterpret_cast<void*>(frame->rip));
    frame = frame->rbp;
  }
  printf("\n");
}
