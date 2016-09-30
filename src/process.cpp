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

void __attribute__((noreturn)) _loop() {
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
  processes = 0;
  nextThread = lastThread = 0;
  uint64_t cpus = ACPI::getController()->getCPUCount();
  cpuThreads = (QueuedThread**)Memory::alloc(sizeof(QueuedThread*) * cpus);
  for (uint64_t c = 0; c < cpus; c++)
    cpuThreads[c] = 0;
  nullThreads = (Thread*)Memory::alloc(sizeof(Thread) * cpus);
}
bool ProcessManager::TimerHandler(uint32_t intr, intcb_regs *regs) {
  return getManager()->SwitchProcess(regs);
}
bool ProcessManager::FaultHandler(uint32_t intr, intcb_regs *regs) {
  return getManager()->KillProcess(intr, regs);
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

bool ProcessManager::KillProcess(uint32_t intr, intcb_regs *regs) {
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
    printf("\nUserspace fault 0x%lx (cpu=%llu)\n"
           "RIP=%016llx RSP=%016llx CS=%04llx SS=%04llx\n"
           "RFL=%016llx [%s]\n"
           "RBP=%016llx RSI=%016llx RDI=%016llx\n"
           "RAX=%016llx RCX=%016llx RDX=%016llx\n"
           "RBX=%016llx R8 =%016llx R9 =%016llx\n"
           "R10=%016llx R11=%016llx R12=%016llx\n"
           "R13=%016llx R14=%016llx R15=%016llx\n",
           intr, regs->cpuid, regs->rip, regs->rsp, regs->cs, regs->ss,
           regs->rflags, rflags_buf, regs->rbp, regs->rsi, regs->rdi, regs->rax,
           regs->rcx, regs->rdx, regs->rbx, regs->r8, regs->r9, regs->r10,
           regs->r11, regs->r12, regs->r13, regs->r14, regs->r15);
  } else {
    printf("Exit\n");
  }
  delete thread->process;
  Memory::free(thread);
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
  uint64_t pcount = 0, pid = 1;
  while (processes != 0 && processes[pcount] != 0) {
    pid = MAX(pid, processes[pcount]->getId() + 1);
    pcount++;
  }
  processes = (Process**)Memory::realloc(processes,
                                         sizeof(Process*) * (pcount + 2));
  processes[pcount + 0] = process;
  processes[pcount + 1] = 0;
  processSwitchMutex.release();
  LeaveCritical(t);
  return pid;
}

void ProcessManager::queueThread(Process *process, Thread *thread) {
  QueuedThread *q = (QueuedThread*)Memory::alloc(sizeof(QueuedThread));
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
    Memory::free(next);
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
  symbols = 0;
  threads = 0;
  entry = 0;
}
Process::~Process() {
  if (pagetable != 0) {
    PTE addr;
    for (uint16_t ptx = 0; ptx < 512; ptx++) {
      addr = pagetable[ptx];
      if (!addr.present)
        continue;
      PPTE ppde = (PPTE)PTE_GET_PTR(addr);
      for (uint16_t pdx = 0; pdx < 512; pdx++) {
        addr = ppde[pdx];
        if (!addr.present)
          continue;
        PPTE pppde = (PPTE)PTE_GET_PTR(addr);
        for (uint16_t pdpx = 0; pdpx < 512; pdpx++) {
          addr = pppde[pdpx];
          if (!addr.present)
            continue;
          PPTE ppml4e = (PPTE)PTE_GET_PTR(addr);
          for (uint16_t pml4x = 0; pml4x < 512; pml4x++) {
            addr = ppml4e[pml4x];
            if (!addr.present)
              continue;
            void *page = PTE_GET_PTR(addr);
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
  if (symbols != 0) {
    uint64_t sid = 0;
    while (symbols[sid].ptr != 0) {
      Memory::free(symbols[sid].name);
      sid++;
    }
  }
  Memory::free(symbols);
  if (threads != 0) {
    uint64_t tid = 0;
    while (threads[tid] != 0) {
      ProcessManager::getManager()->dequeueThread(threads[tid]);
      delete threads[tid];
      tid++;
    }
  }
  Memory::free(threads);
}

uint64_t Process::getId() {
  return id;
}

void Process::addPage(uintptr_t vaddr, void* paddr, uint8_t flags) {
  uint16_t ptx = (vaddr >> (12 + 9 + 9 + 9)) & 0x1FF;
  uint16_t pdx = (vaddr >> (12 + 9 + 9)) & 0x1FF;
  uint16_t pdpx = (vaddr >> (12 + 9)) & 0x1FF;
  uint16_t pml4x = (vaddr >> (12)) & 0x1FF;
  if (pagetable == 0) {
    pagetable = (PPTE)Memory::palloc();
    addPage((uintptr_t)pagetable, pagetable, 5);
  }
  PTE pte = pagetable[ptx];
  if (!pte.present) {
    pagetable[ptx] = pte = PTE_MAKE(Memory::palloc(), 5);
    addPage((uintptr_t)PTE_GET_PTR(pte), PTE_GET_PTR(pte), 5);
  }
  PPTE pde = (PPTE)PTE_GET_PTR(pte);
  pte = pde[pdx];
  if (!pte.present) {
    pde[pdx] = pte = PTE_MAKE(Memory::palloc(), 5);
    addPage((uintptr_t)PTE_GET_PTR(pte), PTE_GET_PTR(pte), 5);
  }
  PPTE pdpe = (PPTE)PTE_GET_PTR(pte);
  pte = pdpe[pdpx];
  if (!pte.present) {
    pdpe[pdpx] = pte = PTE_MAKE(Memory::palloc(), 5);
    addPage((uintptr_t)PTE_GET_PTR(pte), PTE_GET_PTR(pte), 5);
  }
  PPTE pml4e = (PPTE)PTE_GET_PTR(pte);
  flags |= 1;
  pml4e[pml4x] = PTE_MAKE(paddr, flags);
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
  size_t symbolcount = 0;
  ProcessSymbol *old = symbols;
  if (old != 0) {
    while (old[symbolcount].name != 0 && old[symbolcount].ptr != 0)
      symbolcount++;
  }
  symbols = (ProcessSymbol*)Memory::realloc(
      symbols, sizeof(ProcessSymbol) * (symbolcount + 2));
  symbols[symbolcount].ptr = ptr;
  symbols[symbolcount].name = strdup(name);
  symbols[symbolcount + 1].ptr = 0;
  symbols[symbolcount + 1].name = 0;
}
void Process::setEntryAddress(uintptr_t ptr) {
  entry = ptr;
}
uintptr_t Process::getSymbolByName(const char* name) {
  if (symbols == 0)
    return 0;
  size_t idx = 0;
  while (symbols[idx].ptr != 0 && symbols[idx].name != 0) {
    if (strcmp(symbols[idx].name, (char*)name) == 0)
      return symbols[idx].ptr;
    idx++;
  }
  return 0;
}
void Process::writeData(uintptr_t address, void* src, size_t size) {
  while (size > 0) {
    void *dest = getPhysicalAddress(address);
    size_t limit = 0x1000 - ((uintptr_t)dest & 0xFFF);
    size_t count = MIN(size, limit);
    Memory::copy(dest, src, count);
    size -= count;
    src = (void*)((uintptr_t)src + count);
    address += count;
  }
}
void Process::readData(void* dst, uintptr_t address, size_t size) {
  while (size > 0) {
    void *src = getPhysicalAddress(address);
    size_t limit = 0x1000 - ((uintptr_t)src & 0xFFF);
    size_t count = MIN(size, limit);
    Memory::copy(dst, src, count);
    size -= count;
    dst = (void*)((uintptr_t)dst + count);
    address += count;
  }
}
char *Process::readString(uintptr_t address) {
  size_t length = 0;
  char *src = (char*)getPhysicalAddress(address);
  size_t limit = 0x1000 - ((uintptr_t)src & 0xFFF);
  while (limit-- && src[length] != 0) {
    length++;
    if (limit == 0) {
      src = (char*)getPhysicalAddress(address + length);
      limit += 0x1000;
    }
  }
  if (length == 0)
    return 0;
  char *buf = (char*)Memory::alloc(length + 1);
  readData(buf, address, length + 1);
  return buf;
}
uintptr_t Process::getVirtualAddress(void* addr) {
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
  PPTE pde = addr.present ? (PPTE)PTE_GET_PTR(addr) : 0;
  if (pde == 0)
    return 0;
  addr = pde[pdx];
  PPTE pdpe = addr.present ? (PPTE)PTE_GET_PTR(addr) : 0;
  if (pdpe == 0)
    return 0;
  addr = pdpe[pdpx];
  PPTE page = addr.present ? (PPTE)PTE_GET_PTR(addr) : 0;
  if (page == 0)
    return 0;
  addr = page[pml4x];
  void *_ptr = addr.present ? (void*)((uintptr_t)PTE_GET_PTR(addr) + off) : 0;
  return _ptr;
}
void Process::addThread(Thread *thread, bool suspended) {
  if (thread->stack_top == 0) {
    thread->stack_top = addSection(SectionTypeStack, 0x7FFF) + 0x8000;
    thread->regs.rsp = thread->stack_top;
  }
  thread->suspend_ticks = suspended ? -1 : 0;

  uint64_t tcount = 0;
  while (threads != 0 && threads[tcount] != 0)
    tcount++;
  threads = (Thread**)Memory::realloc(threads, sizeof(Thread*) * (tcount + 2));
  threads[tcount + 0] = thread;
  threads[tcount + 1] = 0;
  ProcessManager::getManager()->queueThread(this, thread);
}
extern "C" {
  extern void* __interrupt_wrap;
  extern void* interrupt_handler;
}
void Process::startup() {
  do {
    struct DT {
      uint16_t limit;
      uintptr_t ptr;
    }__attribute__((packed));
    struct GDT_ENT {
      uint64_t seg_lim_low :16;
      uint64_t base_low :24;
      uint8_t type :4;
      bool system :1;
      uint8_t dpl :2;
      bool present :1;
      uint64_t seg_lim_high :4;
      bool avl :1;
      bool islong :1;
      bool db :1;
      bool granularity :1;
      uint64_t base_high :40;
      uint32_t rsvd;
    }__attribute__((packed));
    struct TSS64_ENT {
      uint32_t reserved1;
      uint64_t rsp[3];
      uint64_t reserved2;
      uint64_t ist[7];
      uint64_t reserved3;
      uint16_t reserved4;
      uint16_t iomap_base;
    }__attribute__((packed));
    DT gdt = { 0, 0 };
    DT idt = { 0, 0 };
    asm volatile("sgdtq %0; sidtq %1":"=m"(gdt), "=m"(idt));

    static const uintptr_t KB4 = 0xFFFFFFFFFFFFF000;
    for (uintptr_t addr = gdt.ptr & KB4; addr < (gdt.ptr + gdt.limit); addr +=
        0x1000) {
      addPage(addr, (void*)addr, 5);
    }
    for (uintptr_t addr = idt.ptr & KB4; addr < (idt.ptr + idt.limit); addr +=
        0x1000) {
      addPage(addr, (void*)addr, 5);
    }
    INTERRUPT64 *recs = (INTERRUPT64*)idt.ptr;
    uintptr_t page = 0;
    for (uint16_t i = 0; i < 0x100; i++) {
      uintptr_t handler = ((uintptr_t)recs[i].offset_low
          | ((uintptr_t)recs[i].offset_middle << 16)
          | ((uintptr_t)recs[i].offset_high << 32));
      if (page != (handler & KB4)) {
        page = handler & KB4;
        addPage(page, (void*)page, 5);
      }
    }
    uintptr_t handler = (uintptr_t)&__interrupt_wrap & KB4;
    addPage(handler, (void*)handler, 5);
    handler = (uintptr_t)&interrupt_handler & KB4;
    addPage(handler, (void*)handler, 5);
    GDT_ENT *gdt_ent = (GDT_ENT*)(gdt.ptr + 8 * 3);
    GDT_ENT *gdt_top = (GDT_ENT*)(gdt.ptr + gdt.limit);
    while (gdt_ent < gdt_top) {
      uintptr_t base = gdt_ent->base_low | (gdt_ent->base_high << 24);
      size_t limit = gdt_ent->seg_lim_low | (gdt_ent->seg_lim_high << 16);
      if (((gdt_ent->type != 0x9) && (gdt_ent->type != 0xB)) || (limit
          != sizeof(TSS64_ENT))) {
        gdt_ent++;
        continue;
      }
      uintptr_t page = base & KB4;
      addPage(page, (void*)page, 5);

      TSS64_ENT *tss = (TSS64_ENT*)base;
      uintptr_t stack = tss->ist[0];
      page = stack - 0x1000;
      addPage(page, (void*)page, 5);
      gdt_ent++;
    }
  } while (0);
  Thread *thread = new Thread();
  thread->regs.rip = entry;
  thread->regs.rflags = 0;
  id = (ProcessManager::getManager())->RegisterProcess(this);
  addThread(thread, false);
}
