//    PhoeniX OS Process Manager subsystem
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

#include "processmanager.hpp"

#include "acpi.hpp"

void NORETURN _loop() {
  for (;;)
    asm volatile("hlt");
}

void process_loop() {
  uint32_t cpuid = ACPI::getController()->getCPUID();
  Thread nullThread = Thread();
  asm volatile("mov %%rsp, %0":"=m"(nullThread.regs.rsp));
  nullThread.regs.rip = (uintptr_t)&_loop;
  ProcessManager::getManager()->createNullThread(cpuid, nullThread);
  _loop();
}

ProcessManager* ProcessManager::manager = 0;
static Mutex managerMutex;
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
    const FAULT *f;
    asm volatile("lea FAULTS(%%rip), %q0":"=r"(f));
    f += intr;
    printf("\nUserspace fault %s (cpu=%u, error=0x%x)\n"
           "RIP=%016lx RSP=%016lx CS=%04x SS=%04x\n"
           "RFL=%016lx [%s] CR2=%016lx\n"
           "RBP=%016lx RSI=%016lx RDI=%016lx\n"
           "RAX=%016lx RCX=%016lx RDX=%016lx\n"
           "RBX=%016lx R8 =%016lx R9 =%016lx\n"
           "R10=%016lx R11=%016lx R12=%016lx\n"
           "R13=%016lx R14=%016lx R15=%016lx\n",
           f->code, regs->cpuid, code,
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
