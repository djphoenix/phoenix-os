//    PhoeniX OS Process subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "process.hpp"

#include "processmanager.hpp"
#include "syscall.hpp"

using PTE = Pagetable::Entry;

Process::Process() {
  id = size_t(-1);
  pagetable = nullptr;
  entry = 0;
  _aslrCode = RAND::get<uintptr_t>(0x80000000llu, 0x100000000llu) << 12;
  _aslrStack = RAND::get<uintptr_t>(0x40000000llu, 0x80000000llu) << 12;
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
}

PTE* Process::addPage(uintptr_t vaddr, void* paddr, uint8_t flags) {
  uint16_t ptx = (vaddr >> (12 + 9 + 9 + 9)) & 0x1FF;
  uint16_t pdx = (vaddr >> (12 + 9 + 9)) & 0x1FF;
  uint16_t pdpx = (vaddr >> (12 + 9)) & 0x1FF;
  uint16_t pml4x = (vaddr >> (12)) & 0x1FF;
  if (pagetable == nullptr) {
    pagetable = static_cast<PTE*>(Pagetable::alloc());
    addPage(uintptr_t(pagetable), pagetable, 5);
  }
  PTE pte = pagetable[ptx];
  if (!pte.present) {
    pagetable[ptx] = pte = PTE(Pagetable::alloc(), 7);
    addPage(pte.getUintPtr(), pte.getPtr(), 5);
  }
  PTE *pde = pte.getPTE();
  pte = pde[pdx];
  if (!pte.present) {
    pde[pdx] = pte = PTE(Pagetable::alloc(), 7);
    addPage(pte.getUintPtr(), pte.getPtr(), 5);
  }
  PTE *pdpe = pte.getPTE();
  pte = pdpe[pdpx];
  if (!pte.present) {
    pdpe[pdpx] = pte = PTE(Pagetable::alloc(), 7);
    addPage(pte.getUintPtr(), pte.getPtr(), 5);
  }
  PTE *pml4e = pte.getPTE();
  flags |= 1;
  pml4e[pml4x] = PTE(paddr, flags);
  return &pml4e[pml4x];
}

uintptr_t Process::addSection(SectionType type, size_t size) {
  if (size == 0)
    return 0;
  size_t pages = (size >> 12) + 1;
  uintptr_t vaddr;
  if (type != SectionTypeStack) {
    vaddr = _aslrCode;
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
    uint8_t flags = 4;
    switch (type) {
      case SectionTypeCode:
      case SectionTypeROData:
        break;
      case SectionTypeData:
      case SectionTypeBSS:
      case SectionTypeStack:
        flags |= 2;
        break;
    }
    addPage(vaddr, Pagetable::alloc(), flags)->nx = type != SectionTypeCode;
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
uintptr_t Process::getSymbolByName(const char* name) {
  for (size_t i = 0; i < symbols.getCount(); i++) {
    if (klib::strcmp(symbols[i].name, name) == 0)
      return symbols[i].ptr;
  }
  return 0;
}
uintptr_t Process::linkLibrary(const char* funcname) {
  uintptr_t ptr = getSymbolByName(funcname);
  if (ptr != 0) return ptr;

  uint64_t syscall_id;
  if ((syscall_id = Syscall::callByName(funcname)) != 0) {
    struct {
      uint8_t sbp[4];
      uint8_t pushac11[4];
      uint8_t movabs[2];
      uint64_t syscall_id;
      uint8_t syscall[2];
      uint8_t popac11b[5];
      uint8_t ret;
    } PACKED call = {
      { 0x55, 0x48, 0x89, 0xe5 },
      { 0x50, 0x51, 0x41, 0x53 },
      { 0x48, 0xb8 },
      syscall_id,
      { 0x0f, 0x05 },
      { 0x41, 0x5b, 0x59, 0x58, 0x5d },
      0xc3
    };
    ptr = addSection(SectionTypeCode, sizeof(call));
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
  char *ptr = static_cast<char*>(dst);
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
  const char *src = static_cast<const char*>(getPhysicalAddress(address));
  size_t limit = 0x1000 - (uintptr_t(src) & 0xFFF);
  while (limit-- && src[length] != 0) {
    length++;
    if (limit == 0) {
      src = static_cast<const char*>(getPhysicalAddress(address + length));
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
    thread->stack_top = addSection(SectionTypeStack, 0x7FFF) + 0x8000;
    thread->regs.rsp = thread->stack_top;
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
      map = reinterpret_cast<uint8_t*>(iomap[space] = Pagetable::alloc());
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
    addPage(addr, reinterpret_cast<void*>(addr), 1);
  }
  for (uintptr_t addr = uintptr_t(idt.addr) & KB4;
      addr < (uintptr_t(idt.addr) + idt.limit); addr += 0x1000) {
    addPage(addr, reinterpret_cast<void*>(addr), 1);
  }
  Interrupts::REC64 *recs = static_cast<Interrupts::REC64*>(idt.addr);
  uintptr_t page = 0;
  for (uint16_t i = 0; i < 0x100; i++) {
    uintptr_t handler = (uintptr_t(recs[i].offset_low)
        | (uintptr_t(recs[i].offset_middle) << 16)
        | (uintptr_t(recs[i].offset_high) << 32));
    if (page != (handler & KB4)) {
      page = handler & KB4;
      addPage(page, reinterpret_cast<void*>(page), 1);
    }
  }
  uintptr_t handler, sc_wrapper;
  asm volatile("lea __interrupt_wrap(%%rip), %q0":"=r"(handler));
  asm volatile("lea _ZN7Syscall7wrapperEv(%%rip), %q0":"=r"(sc_wrapper));
  handler &= KB4;
  sc_wrapper &= KB4;
  addPage(handler, reinterpret_cast<void*>(handler), 1);
  addPage(handler + 0x1000, reinterpret_cast<void*>(handler + 0x1000), 1);
  addPage(sc_wrapper, reinterpret_cast<void*>(sc_wrapper), 1);
  addPage(sc_wrapper + 0x1000, reinterpret_cast<void*>(sc_wrapper + 0x1000), 1);
  GDT::Entry *gdt_ent = reinterpret_cast<GDT::Entry*>(uintptr_t(gdt.addr) + 8 * 3);
  GDT::Entry *gdt_top = reinterpret_cast<GDT::Entry*>(uintptr_t(gdt.addr) + gdt.limit);

  if (!iomap[0]) {
    iomap[0] = Pagetable::alloc();
    Memory::fill(iomap[0], 0xFF, 0x1000);
  }
  if (!iomap[1]) {
    iomap[1] = Pagetable::alloc();
    Memory::fill(iomap[1], 0xFF, 0x1000);
  }

  while (gdt_ent < gdt_top) {
    uintptr_t base = gdt_ent->getBase();
    size_t limit = gdt_ent->getLimit();
    if (((gdt_ent->type != 0x9) && (gdt_ent->type != 0xB)) || (limit != sizeof(TSS64_ENT) + 0x2000 - 1)) {
      gdt_ent++;
      continue;
    }

    GDT::SystemEntry *sysent = reinterpret_cast<GDT::SystemEntry*>(gdt_ent);
    base = sysent->getBase();

    uintptr_t page = base & KB4;
    addPage(page, reinterpret_cast<void*>(page), 1);
    addPage(page + 0x1000, iomap[0], 1);
    addPage(page + 0x2000, iomap[1], 1);

    TSS64_ENT *tss = reinterpret_cast<TSS64_ENT*>(base);
    uintptr_t stack = tss->ist[0];
    page = stack - 0x1000;
    addPage(page, reinterpret_cast<void*>(page), 3);
    gdt_ent = reinterpret_cast<GDT::Entry*>(sysent + 1);
  }

  Thread *thread = new Thread();
  thread->regs.rip = entry;
  thread->regs.rflags = 0;
  id = (ProcessManager::getManager())->RegisterProcess(this);
  addThread(thread, false);
}

void Process::exit(int code) {
  printf("EXIT %d\n", code);
  for (size_t i = 0; i < threads.getCount(); i++) {
    ProcessManager::getManager()->dequeueThread(threads[i]);
  }
  delete this;
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
    printf(" [%p]:%p", frame->rbp, reinterpret_cast<void*>(frame->rip));
    frame = frame->rbp;
  }
  printf("\n");
}
