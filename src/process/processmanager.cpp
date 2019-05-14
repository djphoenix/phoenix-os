//    PhoeniX OS Process Manager subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "processmanager.hpp"

#include "acpi.hpp"

void __attribute((naked)) ProcessManager::process_loop() {
  asm volatile(
      "1: hlt; jmp 1b;"
      "process_loop_top:"
      );
}

Mutex ProcessManager::managerMutex;
ProcessManager* ProcessManager::manager = nullptr;

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
  nextThread = lastThread = nullptr;
  uint64_t cpus = ACPI::getController()->getCPUCount();
  cpuThreads = new QueuedThread*[cpus]();
  Interrupts::addCallback(0x20, &ProcessManager::TimerHandler);
  for (uint8_t i = 0; i < 0x20; i++) {
    Interrupts::addCallback(i, &ProcessManager::FaultHandler);
  }
}
bool ProcessManager::TimerHandler(uint8_t, uint32_t, Interrupts::CallbackRegs *regs) {
  return getManager()->SwitchProcess(regs);
}
bool ProcessManager::FaultHandler(uint8_t intr, uint32_t code, Interrupts::CallbackRegs *regs) {
  return getManager()->HandleFault(intr, code, regs);
}
bool ProcessManager::SwitchProcess(Interrupts::CallbackRegs *regs) {
  uintptr_t loopbase = uintptr_t(&process_loop), looptop;
  asm volatile("lea process_loop_top(%%rip), %q0":"=r"(looptop));
  processSwitchMutex.lock();
  if (regs->dpl == 0 &&
      (regs->rip < loopbase || regs->rip >= looptop)) {
    processSwitchMutex.release();
    return false;
  }
  QueuedThread *thread = nextThread;
  if (thread == nullptr) {
    processSwitchMutex.release();
    return false;
  }
  nextThread = thread->next;
  if (thread == lastThread)
    lastThread = nullptr;
  if (cpuThreads[regs->cpuid] != nullptr) {
    QueuedThread *cth = cpuThreads[regs->cpuid];
    Thread *th = cth->thread;
    th->regs = {
      regs->rip, regs->rflags,
      regs->rsi, regs->rdi, regs->rbp, regs->rsp,
      regs->rax, regs->rcx, regs->rdx, regs->rbx,
      regs->r8, regs->r9, regs->r10, regs->r11,
      regs->r12, regs->r13, regs->r14, regs->r15
    };
    if (lastThread != nullptr) {
      lastThread->next = cth;
      lastThread = cth;
    } else {
      nextThread = lastThread = cth;
    }
  }
  cpuThreads[regs->cpuid] = thread;
  Thread *th = thread->thread;
  *regs = {
    regs->cpuid, uintptr_t(thread->process->pagetable),
    th->regs.rip, 0x20,
    th->regs.rflags,
    th->regs.rsp, 0x18,
    3,
    th->regs.rax, th->regs.rcx, th->regs.rdx, th->regs.rbx,
    th->regs.rbp, th->regs.rsi, th->regs.rdi,
    th->regs.r8, th->regs.r9, th->regs.r10, th->regs.r11,
    th->regs.r12, th->regs.r13, th->regs.r14, th->regs.r15
  };
  processSwitchMutex.release();
  return true;
}

bool ProcessManager::HandleFault(uint8_t intr, uint32_t code, Interrupts::CallbackRegs *regs) {
  if (regs->dpl == 0) return false;
  uint64_t t = EnterCritical();
  processSwitchMutex.lock();
  QueuedThread *thread = cpuThreads[regs->cpuid];
  cpuThreads[regs->cpuid] = nullptr;
  processSwitchMutex.release();
  LeaveCritical(t);
  if (!thread) return false;
  Interrupts::print(intr, regs, code, thread->process);
  delete thread->process;
  delete thread;
  if (SwitchProcess(regs))
    return true;
  t = EnterCritical();
  processSwitchMutex.lock();
  asm volatile("mov %%cr3, %0":"=r"(regs->cr3));
  regs->rip = uintptr_t(&process_loop);
  regs->cs = 0x08;
  regs->ss = 0x10;
  regs->dpl = 0;
  processSwitchMutex.release();
  LeaveCritical(t);
  return true;
}

uint64_t ProcessManager::RegisterProcess(Process *process) {
  uint64_t t = EnterCritical();
  processSwitchMutex.lock();
  uint64_t pid = 1;
  for (size_t i = 0; i < processes.getCount(); i++) {
    pid = klib::max(pid, processes[i]->getId());
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
  q->next = nullptr;
  uint64_t t = EnterCritical();
  processSwitchMutex.lock();
  if (lastThread) {
    lastThread = (lastThread->next = q);
  } else {
    lastThread = nextThread = q;
  }
  processSwitchMutex.release();
  LeaveCritical(t);
}

void ProcessManager::dequeueThread(Thread *thread) {
  uint64_t t = EnterCritical();
  processSwitchMutex.lock();
  QueuedThread *next = nextThread, *prev = nullptr;
  while (next != nullptr) {
    if (next->thread != thread) {
      prev = next;
      next = next->next;
      continue;
    }
    if (prev == nullptr)
      nextThread = next->next;
    else
      prev->next = next->next;
    if (lastThread == next)
      lastThread = prev;
    delete next;
    next = prev ? prev->next : nextThread;
  }
  for (size_t cpu = 0; cpu < ACPI::getController()->getCPUCount(); cpu++) {
    if (!cpuThreads[cpu]) continue;
    if (cpuThreads[cpu]->thread != thread) continue;
    delete cpuThreads[cpu];
    cpuThreads[cpu] = nullptr;
  }
  processSwitchMutex.release();
  LeaveCritical(t);
}

Process *ProcessManager::currentProcess() {
  uint64_t cpuid = ACPI::getController()->getCPUID();
  uint64_t t = EnterCritical();
  processSwitchMutex.lock();
  QueuedThread *curr = cpuThreads[cpuid];
  processSwitchMutex.release();
  LeaveCritical(t);
  return curr ? curr->process : nullptr;
}
