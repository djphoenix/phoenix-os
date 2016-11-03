//    PhoeniX OS Processes subsystem
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

#include "process.hpp"

void _loop();
void process_loop() {
  uint32_t cpuid = ACPI::getController()->getCPUID();
  Thread nullThread = Thread();
  asm volatile("mov %%rsp, %0":"=m"(nullThread.regs.rsp));
  nullThread.regs.rip = (uintptr_t)&_loop;
  ProcessManager::getManager()->createNullThread(cpuid, nullThread);
  _loop();
}

void NORETURN _loop() {
  for (;;)
    asm volatile("hlt");
}

ProcessManager* ProcessManager::manager = 0;
Mutex managerMutex = Mutex();
ProcessManager* ProcessManager::getManager() {
  if (manager) return manager;
  uint64_t t = EnterCritical();
  managerMutex.lock();
  if (!manager)
    manager = new ProcessManager();
  managerMutex.release();
  LeaveCritical(t);
  return manager;
}
ProcessManager::ProcessManager() {
  Interrupts::addCallback(0x20, &ProcessManager::TimerHandler);
  for (int i = 0; i < 0x20; i++) {
    Interrupts::addCallback(i, &ProcessManager::FaultHandler);
  }
  processSwitchMutex = Mutex();
  nextThread = lastThread = 0;
  uint64_t cpus = ACPI::getController()->getCPUCount();
  cpuThreads = new QueuedThread*[cpus]();
  for (uint64_t c = 0; c < cpus; c++)
    cpuThreads[c] = 0;
  nullThreads = new Thread[cpus]();
}
bool ProcessManager::TimerHandler(uint32_t, uint32_t, intcb_regs *regs) {
  return getManager()->SwitchProcess(regs);
}
bool ProcessManager::FaultHandler(
    uint32_t intr, uint32_t code, intcb_regs *regs) {
  return getManager()->HandleFault(intr, code, regs);
}
void ProcessManager::createNullThread(uint32_t cpuid, Thread thread) {
  uint64_t t = EnterCritical();
  processSwitchMutex.lock();
  nullThreads[cpuid] = thread;
  processSwitchMutex.release();
  LeaveCritical(t);
}
bool ProcessManager::SwitchProcess(intcb_regs *regs) {
  processSwitchMutex.lock();
  if (nullThreads[regs->cpuid].regs.rip != (uintptr_t)&_loop) {
    processSwitchMutex.release();
    return false;
  }
  QueuedThread *thread = nextThread;
  if (thread == 0) {
    processSwitchMutex.release();
    return false;
  }
  nextThread = thread->next;
  if (thread == lastThread)
    lastThread = 0;
  if (cpuThreads[regs->cpuid] != 0) {
    QueuedThread *cth = cpuThreads[regs->cpuid];
    Thread *th = cth->thread;
    th->regs = {
      regs->rip, regs->rflags,
      regs->rsi, regs->rdi, regs->rbp, regs->rsp,
      regs->rax, regs->rcx, regs->rdx, regs->rbx,
      regs->r8, regs->r9, regs->r10, regs->r11,
      regs->r12, regs->r13, regs->r14, regs->r15
    };
    if (lastThread != 0) {
      lastThread->next = cth;
      lastThread = cth;
    } else {
      nextThread = lastThread = cth;
    }
  }
  cpuThreads[regs->cpuid] = thread;
  Thread *th = thread->thread;
  *regs = {
    regs->cpuid, (uintptr_t)thread->process->pagetable,
    th->regs.rip, 0x18,
    th->regs.rflags,
    th->regs.rsp, 0x20,
    3,
    th->regs.rax, th->regs.rcx, th->regs.rdx, th->regs.rbx,
    th->regs.rbp, th->regs.rsi, th->regs.rdi,
    th->regs.r8, th->regs.r9, th->regs.r10, th->regs.r11,
    th->regs.r12, th->regs.r13, th->regs.r14, th->regs.r15
  };
  processSwitchMutex.release();
  return true;
}

bool ProcessManager::HandleFault(
    uint32_t intr, uint32_t code, intcb_regs *regs) {
  if (regs->dpl != 3)
    return false;
  uint64_t t = EnterCritical();
  processSwitchMutex.lock();
  QueuedThread *thread = cpuThreads[regs->cpuid];
  cpuThreads[regs->cpuid] = 0;
  processSwitchMutex.release();
  LeaveCritical(t);
  uint8_t instrbuf[3];
  thread->process->readData(instrbuf, regs->rip, 3);
  if ((intr != 0x0E) || (regs->rsp != thread->thread->stack_top)
      || !((instrbuf[0] == 0xF3 && instrbuf[1] == 0xC3) ||  // repz ret
          (instrbuf[0] == 0xC3) ||  // ret
          false)
      || false) {
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
    uint64_t cr2;
    asm volatile("mov %%cr2, %0":"=a"(cr2));
    printf("\nUserspace fault %s (cpu=%llu, error=0x%lx)\n"
           "RIP=%016llx RSP=%016llx CS=%04llx SS=%04llx\n"
           "RFL=%016llx [%s] CR2=%016llx\n"
           "RBP=%016llx RSI=%016llx RDI=%016llx\n"
           "RAX=%016llx RCX=%016llx RDX=%016llx\n"
           "RBX=%016llx R8 =%016llx R9 =%016llx\n"
           "R10=%016llx R11=%016llx R12=%016llx\n"
           "R13=%016llx R14=%016llx R15=%016llx\n",
           FAULTS[intr].code, regs->cpuid, code,
           regs->rip, regs->rsp, regs->cs, regs->ss,
           regs->rflags, rflags_buf, cr2,
           regs->rbp, regs->rsi, regs->rdi,
           regs->rax, regs->rcx, regs->rdx, regs->rbx,
           regs->r8, regs->r9, regs->r10, regs->r11,
           regs->r12, regs->r13, regs->r14, regs->r15);
  } else {
    printf("Exit\n");
  }
  delete thread->process;
  delete thread;
  if (SwitchProcess(regs))
    return true;
  t = EnterCritical();
  processSwitchMutex.lock();
  Thread *th = &nullThreads[regs->cpuid];
  uintptr_t pagetable = 0;
  asm volatile("mov %%cr3, %0":"=r"(pagetable));
  *regs = {
    regs->cpuid, pagetable,
    th->regs.rip, 0x08,
    th->regs.rflags,
    th->regs.rsp, 0x10,
    0,
    th->regs.rax, th->regs.rcx, th->regs.rdx, th->regs.rbx,
    th->regs.rbp, th->regs.rsi, th->regs.rdi,
    th->regs.r8, th->regs.r9, th->regs.r10, th->regs.r11,
    th->regs.r12, th->regs.r13, th->regs.r14, th->regs.r15
  };
  processSwitchMutex.release();
  LeaveCritical(t);
  return true;
}

uint64_t ProcessManager::RegisterProcess(Process *process) {
  uint64_t t = EnterCritical();
  processSwitchMutex.lock();
  uint64_t pid = 1;
  for (size_t i = 0; i < processes.getCount(); i++) {
    pid = MAX(pid, processes[i]->getId());
  }
  processes.add(process);
  processSwitchMutex.release();
  LeaveCritical(t);
  return pid;
}

void ProcessManager::queueThread(Process *process, Thread *thread) {
  QueuedThread *q = new QueuedThread();
  q->process = process;
  q->thread = thread;
  q->next = 0;
  uint64_t t = EnterCritical();
  processSwitchMutex.lock();
  if (lastThread)
    lastThread->next = q;
  else
    lastThread = nextThread = q;
  processSwitchMutex.release();
  LeaveCritical(t);
}

void ProcessManager::dequeueThread(Thread *thread) {
  uint64_t t = EnterCritical();
  processSwitchMutex.lock();
  QueuedThread *next = nextThread, *prev = 0;
  while (next != 0) {
    if (next->thread != thread) {
      prev = next;
      next = next->next;
      continue;
    }
    if (prev == 0)
      nextThread = next->next;
    else
      prev->next = next->next;
    if (lastThread == next)
      lastThread = prev;
    delete next;
    next = prev ? prev->next : nextThread;
  }
  processSwitchMutex.release();
  LeaveCritical(t);
}

Thread::Thread() {
  regs = {
    0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
  };
  suspend_ticks = 0;
  stack_top = 0;
}

Process::Process() {
  id = -1;
  pagetable = 0;
  entry = 0;
}
Process::~Process() {
  if (pagetable != 0) {
    PTE addr;
    for (uint16_t ptx = 0; ptx < 512; ptx++) {
      addr = pagetable[ptx];
      if (!addr.present)
        continue;
      PTE *ppde = addr.getPTE();
      for (uint16_t pdx = 0; pdx < 512; pdx++) {
        addr = ppde[pdx];
        if (!addr.present)
          continue;
        PTE *pppde = addr.getPTE();
        for (uint16_t pdpx = 0; pdpx < 512; pdpx++) {
          addr = pppde[pdpx];
          if (!addr.present)
            continue;
          PTE *ppml4e = addr.getPTE();
          for (uint16_t pml4x = 0; pml4x < 512; pml4x++) {
            addr = ppml4e[pml4x];
            if (!addr.present)
              continue;
            void *page = addr.getPtr();
            if ((uintptr_t)page == (((uintptr_t)ptx << (12 + 9 + 9 + 9))
                | ((uintptr_t)pdx << (12 + 9 + 9))
                | ((uintptr_t)pdpx << (12 + 9)) | ((uintptr_t)pml4x << (12))))
              continue;
            Memory::pfree(page);
          }
          Memory::pfree(ppml4e);
        }
        Memory::pfree(pppde);
      }
      Memory::pfree(ppde);
    }
    Memory::pfree(pagetable);
  }
  for (size_t i = 0; i < symbols.getCount(); i++) {
    delete[] symbols[i].name;
  }
  for (size_t i = 0; i < threads.getCount(); i++) {
    ProcessManager::getManager()->dequeueThread(threads[i]);
    delete threads[i];
  }
}

void Process::addPage(uintptr_t vaddr, void* paddr, uint8_t flags) {
  uint16_t ptx = (vaddr >> (12 + 9 + 9 + 9)) & 0x1FF;
  uint16_t pdx = (vaddr >> (12 + 9 + 9)) & 0x1FF;
  uint16_t pdpx = (vaddr >> (12 + 9)) & 0x1FF;
  uint16_t pml4x = (vaddr >> (12)) & 0x1FF;
  if (pagetable == 0) {
    pagetable = static_cast<PTE*>(Memory::palloc());
    addPage((uintptr_t)pagetable, pagetable, 5);
  }
  PTE pte = pagetable[ptx];
  if (!pte.present) {
    pagetable[ptx] = pte = PTE(Memory::palloc(), 7);
    addPage(pte.getUintPtr(), pte.getPtr(), 5);
  }
  PTE *pde = pte.getPTE();
  pte = pde[pdx];
  if (!pte.present) {
    pde[pdx] = pte = PTE(Memory::palloc(), 7);
    addPage(pte.getUintPtr(), pte.getPtr(), 5);
  }
  PTE *pdpe = pte.getPTE();
  pte = pdpe[pdpx];
  if (!pte.present) {
    pdpe[pdpx] = pte = PTE(Memory::palloc(), 7);
    addPage(pte.getUintPtr(), pte.getPtr(), 5);
  }
  PTE *pml4e = pte.getPTE();
  flags |= 1;
  pml4e[pml4x] = PTE(paddr, flags);
}

uintptr_t Process::addSection(SectionType type, size_t size) {
  if (size == 0)
    return 0;
  size_t pages = (size >> 12) + 1;
  uintptr_t vaddr = 0xF000000000;
  if (type == SectionTypeStack)
    vaddr = 0xA000000000;
  check: for (uintptr_t caddr = vaddr; caddr < vaddr + size; caddr += 0x1000) {
    if (getPhysicalAddress(vaddr) != 0) {
      vaddr += 0x1000;
      goto check;
    }
  }
  uintptr_t addr = vaddr;
  while (pages-- > 0) {
    uint8_t flags = 4;
    switch (type) {
      case SectionTypeCode:
        break;
      case SectionTypeData:
      case SectionTypeBSS:
      case SectionTypeStack:
        flags |= 2;
        break;
    }
    addPage(vaddr, Memory::palloc(), flags);
    vaddr += 0x1000;
  }
  return addr;
}
void Process::addSymbol(const char *name, uintptr_t ptr) {
  symbols.insert() = { ptr, strdup(name) };
}
void Process::setEntryAddress(uintptr_t ptr) {
  entry = ptr;
}
uintptr_t Process::getSymbolByName(const char* name) {
  for (size_t i = 0; i < symbols.getCount(); i++) {
    if (strcmp(symbols[i].name, name) == 0)
      return symbols[i].ptr;
  }
  return 0;
}
void Process::writeData(uintptr_t address, void* src, size_t size) {
  char *ptr = static_cast<char*>(src);
  while (size > 0) {
    void *dest = getPhysicalAddress(address);
    size_t limit = 0x1000 - ((uintptr_t)dest & 0xFFF);
    size_t count = MIN(size, limit);
    Memory::copy(dest, ptr, count);
    size -= count;
    ptr += count;
    address += count;
  }
}
void Process::readData(void* dst, uintptr_t address, size_t size) {
  char *ptr = static_cast<char*>(dst);
  while (size > 0) {
    void *src = getPhysicalAddress(address);
    size_t limit = 0x1000 - ((uintptr_t)src & 0xFFF);
    size_t count = MIN(size, limit);
    Memory::copy(ptr, src, count);
    size -= count;
    ptr += count;
    address += count;
  }
}
char *Process::readString(uintptr_t address) {
  size_t length = 0;
  const char *src = static_cast<const char*>(getPhysicalAddress(address));
  size_t limit = 0x1000 - ((uintptr_t)src & 0xFFF);
  while (limit-- && src[length] != 0) {
    length++;
    if (limit == 0) {
      src = static_cast<const char*>(getPhysicalAddress(address + length));
      limit += 0x1000;
    }
  }
  if (length == 0)
    return 0;
  char *buf = new char[length + 1]();
  readData(buf, address, length + 1);
  return buf;
}
uintptr_t Process::getVirtualAddress(void* addr) {
  (void)addr;
  // TODO: make this work
  return 0;
}
void *Process::getPhysicalAddress(uintptr_t ptr) {
  if (pagetable == 0)
    return 0;
  uint16_t ptx = (ptr >> (12 + 9 + 9 + 9)) & 0x1FF;
  uint16_t pdx = (ptr >> (12 + 9 + 9)) & 0x1FF;
  uint16_t pdpx = (ptr >> (12 + 9)) & 0x1FF;
  uint16_t pml4x = (ptr >> (12)) & 0x1FF;
  uintptr_t off = ptr & 0xFFF;
  PTE addr;
  addr = pagetable[ptx];
  PTE *pde = addr.present ? addr.getPTE() : 0;
  if (pde == 0)
    return 0;
  addr = pde[pdx];
  PTE *pdpe = addr.present ? addr.getPTE() : 0;
  if (pdpe == 0)
    return 0;
  addr = pdpe[pdpx];
  PTE *page = addr.present ? addr.getPTE() : 0;
  if (page == 0)
    return 0;
  addr = page[pml4x];
  return addr.present ? reinterpret_cast<void*>(addr.getUintPtr() + off) : 0;
}
void Process::addThread(Thread *thread, bool suspended) {
  if (thread->stack_top == 0) {
    thread->stack_top = addSection(SectionTypeStack, 0x7FFF) + 0x8000;
    thread->regs.rsp = thread->stack_top;
  }
  thread->suspend_ticks = suspended ? -1 : 0;

  threads.add(thread);
  ProcessManager::getManager()->queueThread(this, thread);
}
extern "C" {
  extern void* __interrupt_wrap;
  extern void* interrupt_handler;
}
void Process::startup() {
  DTREG gdt = { 0, 0 };
  DTREG idt = { 0, 0 };
  asm volatile("sgdtq %0; sidtq %1":"=m"(gdt), "=m"(idt));

  static const uintptr_t KB4 = 0xFFFFFFFFFFFFF000;
  for (uintptr_t addr = (uintptr_t)gdt.addr & KB4;
      addr < ((uintptr_t)gdt.addr + gdt.limit); addr += 0x1000) {
    addPage(addr, reinterpret_cast<void*>(addr), 5);
  }
  for (uintptr_t addr = (uintptr_t)idt.addr & KB4;
      addr < ((uintptr_t)idt.addr + idt.limit); addr += 0x1000) {
    addPage(addr, reinterpret_cast<void*>(addr), 5);
  }
  INTERRUPT64 *recs = static_cast<INTERRUPT64*>(idt.addr);
  uintptr_t page = 0;
  for (uint16_t i = 0; i < 0x100; i++) {
    uintptr_t handler = ((uintptr_t)recs[i].offset_low
        | ((uintptr_t)recs[i].offset_middle << 16)
        | ((uintptr_t)recs[i].offset_high << 32));
    if (page != (handler & KB4)) {
      page = handler & KB4;
      addPage(page, reinterpret_cast<void*>(page), 5);
    }
  }
  uintptr_t handler = (uintptr_t)&__interrupt_wrap & KB4;
  addPage(handler, reinterpret_cast<void*>(handler), 5);
  handler = (uintptr_t)&interrupt_handler & KB4;
  addPage(handler, reinterpret_cast<void*>(handler), 5);
  GDT_ENT *gdt_ent = reinterpret_cast<GDT_ENT*>(
      (uintptr_t)gdt.addr + 8 * 3);
  GDT_ENT *gdt_top = reinterpret_cast<GDT_ENT*>(
      (uintptr_t)gdt.addr + gdt.limit);
  while (gdt_ent < gdt_top) {
    uintptr_t base = gdt_ent->getBase();
    size_t limit = gdt_ent->getLimit();
    if (((gdt_ent->type != 0x9) && (gdt_ent->type != 0xB)) || (limit
        != sizeof(TSS64_ENT))) {
      gdt_ent++;
      continue;
    }
    uintptr_t page = base & KB4;
    addPage(page, reinterpret_cast<void*>(page), 5);

    GDT_SYS_ENT *sysent = reinterpret_cast<GDT_SYS_ENT*>(gdt_ent);
    base = sysent->getBase();

    TSS64_ENT *tss = reinterpret_cast<TSS64_ENT*>(base);
    uintptr_t stack = tss->ist[0];
    page = stack - 0x1000;
    addPage(page, reinterpret_cast<void*>(page), 5);
    gdt_ent = reinterpret_cast<GDT_ENT*>(sysent + 1);
  }

  Thread *thread = new Thread();
  thread->regs.rip = entry;
  thread->regs.rflags = 0;
  id = (ProcessManager::getManager())->RegisterProcess(this);
  addThread(thread, false);
}
