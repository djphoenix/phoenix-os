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
  static ProcessManager* getManager();
  static void NORETURN process_loop();
};
