//    PhoeniX OS Process Manager subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"
#include "thread.hpp"
#include "process.hpp"
#include "interrupts.hpp"

class ProcessManager {
 private:
  struct QueuedThread {
    Process *process;
    Thread *thread;
    QueuedThread* next;
  };

  static Mutex managerMutex;
  static volatile ProcessManager* manager;

  ProcessManager();
  QueuedThread *nextThread, *lastThread;
  QueuedThread **cpuThreads;
  List<Process*> processes;
  Mutex processSwitchMutex;
  bool SwitchProcess(Interrupts::CallbackRegs *regs);
  bool HandleFault(uint8_t intr, uint32_t code, Interrupts::CallbackRegs *regs);
  static bool TimerHandler(uint8_t intr, uint32_t code, Interrupts::CallbackRegs *regs);
  static bool FaultHandler(uint8_t intr, uint32_t code, Interrupts::CallbackRegs *regs);

 public:
  uint64_t RegisterProcess(Process *process);
  void queueThread(Process *process, Thread *thread);
  void dequeueThread(Thread *thread);
  Process *currentProcess();
  static ProcessManager* getManager();
  static void NORETURN process_loop();
};
