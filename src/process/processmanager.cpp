//    PhoeniX OS Process Manager subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "processmanager.hpp"
#include "thread.hpp"

#include "acpi.hpp"

void __attribute((naked)) ProcessManager::process_loop() {
  asm volatile(
      "1: hlt; jmp 1b;"
      "process_loop_top:"
      );
}

Mutex ProcessManager::managerMutex;
volatile ProcessManager* ProcessManager::manager = nullptr;

ProcessManager* ProcessManager::getManager() {
  if (manager) return const_cast<ProcessManager*>(manager);
  Mutex::CriticalLock lock(managerMutex);
  if (!manager) manager = new ProcessManager();
  return const_cast<ProcessManager*>(manager);
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
  Mutex::Lock lock(processSwitchMutex);
  if (regs->info->dpl == 0 && (regs->info->rip < loopbase || regs->info->rip >= looptop)) return false;
  QueuedThread *thread = nextThread;
  if (thread == nullptr) return false;
  nextThread = thread->next;
  if (thread == lastThread)
    lastThread = nullptr;
  if (cpuThreads[regs->cpuid] != nullptr) {
    QueuedThread *cth = cpuThreads[regs->cpuid];
    Thread *th = cth->thread;
    th->rip = regs->info->rip;
    th->rflags = regs->info->rflags;
    th->rsp = regs->info->rsp;
    th->regs = *regs->general;
    th->sse = *regs->sse;
    if (lastThread != nullptr) {
      lastThread->next = cth;
      lastThread = cth;
    } else {
      nextThread = lastThread = cth;
    }
  }
  cpuThreads[regs->cpuid] = thread;
  Thread *th = thread->thread;
  *regs->info = {
    uintptr_t(thread->process->pagetable),
    th->rip, th->rflags, th->rsp,
    0x20, 0x18, 3,
  };
  *regs->general = th->regs;
  *regs->sse = th->sse;
  return true;
}

bool ProcessManager::HandleFault(uint8_t intr, uint32_t code, Interrupts::CallbackRegs *regs) {
  if (regs->info->dpl == 0) return false;
  QueuedThread *thread;
  {
    Mutex::CriticalLock lock(processSwitchMutex);
    thread = cpuThreads[regs->cpuid];
    cpuThreads[regs->cpuid] = nullptr;
  }
  if (!thread) return false;
  Interrupts::print(intr, regs, code, thread->process);
  delete thread->process;
  delete thread;
  if (SwitchProcess(regs)) return true;
  {
    Mutex::CriticalLock lock(processSwitchMutex);
    asm volatile("mov %%cr3, %0":"=r"(regs->info->cr3));
    regs->info->rip = uintptr_t(&process_loop);
    regs->sse->sse[3] = 0x0000ffff00001F80llu;
    regs->info->cs = 0x08;
    regs->info->ss = 0x10;
    regs->info->dpl = 0;
  }
  return true;
}

uint64_t ProcessManager::registerProcess(Process *process) {
  Mutex::CriticalLock lock(processSwitchMutex);
  uint64_t pid = 1;
  for (size_t i = 0; i < processes.getCount(); i++) {
    pid = klib::max(pid, processes[i]->getId() + 1);
  }
  processes.add(process);
  return pid;
}

void ProcessManager::exitProcess(Process *process, int code) {
  (void)code;  // TODO: handle
  Mutex::CriticalLock lock(processSwitchMutex);
  for (size_t i = processes.getCount(); i > 0; i--) {
    if (processes[i-1] == process) processes.remove(i-1);
  }
}

void ProcessManager::queueThread(Process *process, Thread *thread) {
  QueuedThread *q = new QueuedThread();
  q->process = process;
  q->thread = thread;
  q->next = nullptr;
  Mutex::CriticalLock lock(processSwitchMutex);
  if (lastThread) {
    lastThread = (lastThread->next = q);
  } else {
    lastThread = nextThread = q;
  }
}

void ProcessManager::dequeueThread(Thread *thread) {
  Mutex::CriticalLock lock(processSwitchMutex);
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
}

Process *ProcessManager::currentProcess() {
  uint64_t cpuid = ACPI::getController()->getCPUID();
  Mutex::CriticalLock lock(processSwitchMutex);
  QueuedThread *curr = cpuThreads[cpuid];
  return curr ? curr->process : nullptr;
}
