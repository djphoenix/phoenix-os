//    PhoeniX OS Process Manager subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "scheduler.hpp"
#include "thread.hpp"
#include "process.hpp"

#include "acpi.hpp"

#include "sprintf.hpp"
#include "kprint.hpp"

void __attribute((naked)) Scheduler::process_loop() {
  asm volatile(
      "1: hlt; jmp 1b;"
      "process_loop_top:"
      );
}

struct Scheduler::QueuedThread {
  Process *process;
  Thread *thread;
  QueuedThread* next;
};

Scheduler Scheduler::scheduler {};

void Scheduler::setup() { scheduler.init(); }
void Scheduler::init() {
  m_pid = 0;
  uint64_t cpus = ACPI::getController()->getCPUCount();
  cpuThreads = reinterpret_cast<QueuedThread**>(Pagetable::alloc(
    (sizeof(QueuedThread*) * cpus + 0xFFF) / 0x1000,
    Pagetable::MemoryType::DATA_RW
  ));
  Interrupts::addCallback(0x20, &Scheduler::TimerHandler);
  for (uint8_t i = 0; i < 0x20; i++) {
    Interrupts::addCallback(i, &Scheduler::FaultHandler);
  }
}
bool Scheduler::TimerHandler(uint8_t, uint32_t, Interrupts::CallbackRegs *regs) {
  return getScheduler()->SwitchProcess(regs);
}
bool Scheduler::FaultHandler(uint8_t intr, uint32_t code, Interrupts::CallbackRegs *regs) {
  return getScheduler()->HandleFault(intr, code, regs);
}
bool Scheduler::SwitchProcess(Interrupts::CallbackRegs *regs) {
  uintptr_t loopbase = uintptr_t(&process_loop), looptop;
  asm volatile("lea process_loop_top(%%rip), %q0":"=X"(looptop));
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

void Scheduler::PrintFault(uint8_t num, Interrupts::CallbackRegs *regs, uint32_t code, const Process *process) {
  uint64_t cr2;
  uint64_t base;
  asm volatile("mov %%cr2, %0":"=X"(cr2));
  uint32_t cpuid = ACPI::getController()->getCPUID();
  char rflags_buf[10] = "---------";
  if (regs->info->rflags & (1 <<  0)) rflags_buf[8] = 'C';
  if (regs->info->rflags & (1 <<  2)) rflags_buf[7] = 'P';
  if (regs->info->rflags & (1 <<  4)) rflags_buf[6] = 'A';
  if (regs->info->rflags & (1 <<  6)) rflags_buf[5] = 'Z';
  if (regs->info->rflags & (1 <<  7)) rflags_buf[4] = 'S';
  if (regs->info->rflags & (1 <<  8)) rflags_buf[3] = 'T';
  if (regs->info->rflags & (1 <<  9)) rflags_buf[2] = 'I';
  if (regs->info->rflags & (1 << 10)) rflags_buf[1] = 'D';
  if (regs->info->rflags & (1 << 11)) rflags_buf[0] = 'O';
  const char *src = regs->info->dpl ? "Userspace" : (process ? "Syscall" : "Kernel");

  char printbuf[1024];
  char *printbuf_top = printbuf + sizeof(printbuf);
  char *printbuf_ptr = printbuf;

  if (process) {
    base = uintptr_t(process->getBase());
    printbuf_ptr += snprintf(
      printbuf_ptr, size_t(printbuf_top - printbuf_ptr),
      "\n%s (%s) ", src, process->getName()
    );
  } else {
    asm volatile("lea __text_start__(%%rip), %q0":"=X"(base));
    printbuf_ptr += snprintf(
      printbuf_ptr, size_t(printbuf_top - printbuf_ptr),
      "\n%s ", src
    );
  }
  printbuf_ptr += snprintf(
    printbuf_ptr, size_t(printbuf_top - printbuf_ptr),
    "fault %s (cpu=%u, error=0x%x)\n"
    "BASE=%016lx CS=%04hx SS=%04hx DPL=%hhu\n"
    "IP=%016lx FL=%016lx [%s]\n"
    "SP=%016lx BP=%016lx CR2=%016lx\n",
    Interrupts::faults[num].code, cpuid, code,
    base, regs->info->cs, regs->info->ss, regs->info->dpl,
    regs->info->rip, regs->info->rflags, rflags_buf,
    regs->info->rsp, regs->general->rbp, cr2
  );
  printbuf_ptr += snprintf(
    printbuf_ptr, size_t(printbuf_top - printbuf_ptr),
    "SI=%016lx DI=%016lx CR3=%016lx\n"
    "A =%016lx C =%016lx D =%016lx B =%016lx\n"
    "8 =%016lx 9 =%016lx 10=%016lx 11=%016lx\n"
    "12=%016lx 13=%016lx 14=%016lx 15=%016lx\n"
    "\nSTACK TRACE: ",
    regs->general->rsi, regs->general->rdi, regs->info->cr3,
    regs->general->rax, regs->general->rcx, regs->general->rdx, regs->general->rbx,
    regs->general->r8 , regs->general->r9 , regs->general->r10, regs->general->r11,
    regs->general->r12, regs->general->r13, regs->general->r14, regs->general->r15
  );
  printbuf_ptr += Process::print_stacktrace(printbuf_ptr, size_t(printbuf_top - printbuf_ptr) - 3, regs->general->rbp, process);
  *(printbuf_ptr++) = '\n';
  *(printbuf_ptr++) = '\n';
  *(printbuf_ptr++) = 0;
  kprint(printbuf);
}

bool Scheduler::HandleFault(uint8_t intr, uint32_t code, Interrupts::CallbackRegs *regs) {
  QueuedThread *thread;
  {
    Mutex::Lock lock(processSwitchMutex);
    thread = cpuThreads[regs->cpuid];
    cpuThreads[regs->cpuid] = nullptr;
  }
  if (!thread) {
    PrintFault(intr, regs, code, nullptr);
    return false;
  } else {
    PrintFault(intr, regs, code, thread->process);
  }
  delete thread->process;
  delete thread;
  if (SwitchProcess(regs)) return true;
  {
    Mutex::Lock lock(processSwitchMutex);
    asm volatile("mov %%cr3, %0":"=X"(regs->info->cr3));
    regs->info->rip = uintptr_t(&process_loop);
    regs->sse->sse[3] = 0x0000ffff00001F80llu;
    regs->info->cs = 0x08;
    regs->info->ss = 0x10;
    regs->info->dpl = 0;
  }
  return true;
}

uint64_t Scheduler::registerProcess(Process *process) {
  Thread *thread = new Thread();
  thread->rip = process->entry;
  thread->sse.sse[3] = 0x0000ffff00001F80llu;
  thread->rflags = 0;
  thread->stack_top = process->addSection(0, SectionTypeStack, 0x1FFFFF) + 0x200000 - 0x8;
  thread->rsp = thread->stack_top;
  thread->suspend_ticks = 0;

  QueuedThread *q = new QueuedThread();
  q->process = process;
  q->thread = thread;
  q->next = nullptr;

  Mutex::CriticalLock lock(processSwitchMutex);
  uint64_t pid = ++m_pid;
  for (size_t i = 0; i < processes.getCount(); i++) {
    size_t ipid = processes[i]->getId();
    if (ipid >= pid) pid = ipid + 1;
  }
  process->id = pid;
  processes.add(process);
  process->threads.add(thread);

  lastThread = lastThread ? (lastThread->next = q) : (nextThread = q);

  return pid;
}

void __attribute__((noreturn)) Scheduler::exitProcess(Process *process, int code) {
  (void)code;  // TODO: handle

  {
    Mutex::Lock lock(processSwitchMutex);

    QueuedThread *next = nextThread, *prev = nullptr;
    while (next != nullptr) {
      if (next->process != process) {
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
      if (cpuThreads[cpu]->process != process) continue;
      delete cpuThreads[cpu];
      cpuThreads[cpu] = nullptr;
    }

    for (size_t i = processes.getCount(); i > 0; i--) {
      if (processes[i-1] == process) processes.remove(i-1);
    }
  }
  delete process;
  asm("sti; xor %rbp, %rbp");
  process_loop();
}

void Scheduler::queueThread(Process *process, Thread *thread) {
  QueuedThread *q = new QueuedThread();
  q->process = process;
  q->thread = thread;
  q->next = nullptr;
  Mutex::CriticalLock lock(processSwitchMutex);
  lastThread = lastThread ? (lastThread->next = q) : (nextThread = q);
}

void Scheduler::dequeueThread(Thread *thread) {
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

Process *Scheduler::currentProcess() {
  uint64_t cpuid = ACPI::getController()->getCPUID();
  Mutex::CriticalLock lock(processSwitchMutex);
  QueuedThread *curr = cpuThreads[cpuid];
  return curr ? curr->process : nullptr;
}
