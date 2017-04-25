//    PhoeniX OS Process Manager subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"
#include "thread.hpp"
#include "process.hpp"
#include "interrupts.hpp"

struct QueuedThread {
  Process *process;
  Thread *thread;
  QueuedThread* next;
};

class ProcessManager {
 private:
  static Mutex managerMutex;
  static ProcessManager* manager;

  ProcessManager();
  QueuedThread *nextThread, *lastThread;
  QueuedThread **cpuThreads;
  List<Process*> processes;
  Mutex processSwitchMutex;
  bool SwitchProcess(intcb_regs *regs);
  bool HandleFault(uint32_t intr, uint32_t code, intcb_regs *regs);
  static bool TimerHandler(uint32_t intr, uint32_t code, intcb_regs *regs);
  static bool FaultHandler(uint32_t intr, uint32_t code, intcb_regs *regs);

 public:
  uint64_t RegisterProcess(Process *process);
  void queueThread(Process *process, Thread *thread);
  void dequeueThread(Thread *thread);
  Process *currentProcess();
  static ProcessManager* getManager();
  static void NORETURN process_loop();
};
