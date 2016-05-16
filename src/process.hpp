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

#pragma once
#include "pxlib.hpp"
#include "memory.hpp"
extern void __attribute__((noreturn)) process_loop();
typedef struct {
	uintptr_t ptr;
	char* name;
} ProcessSymbol;
class Process;
class Thread;
struct QueuedThread {
	Process *process;
	Thread *thread;
	QueuedThread* next;
};
class ProcessManager {
private:
	ProcessManager();
	QueuedThread *nextThread, *lastThread;
	QueuedThread **cpuThreads;
	Thread *nullThreads;
	Process** processes;
	Mutex processSwitchMutex;
	static ProcessManager* manager;
	bool SwitchProcess(intcb_regs *regs);
	static bool TimerHandler(uint32_t intr, intcb_regs *regs);
public:
	uint64_t RegisterProcess(Process *process);
	void createNullThread(uint32_t cpuid, Thread thread);
	void queueThread(Process *process, Thread *thread);
	void dequeueThread(Thread *thread);
	static ProcessManager* getManager();
};

class Thread {
public:
	Thread();
	struct {
		uint64_t rip, rflags;
		uint64_t rsi, rdi, rbp, rsp;
		uint64_t rax, rcx, rdx, rbx;
		uint64_t r8 , r9 , r10, r11;
		uint64_t r12, r13, r14, r15;
	} regs;
	uint64_t suspend_ticks;
	uint64_t stack_top;
};

typedef enum : uint8_t {
	SectionTypeCode,
	SectionTypeData,
	SectionTypeBSS,
	SectionTypeStack,
} SectionType;

class Process {
private:
	uint64_t id;
	Thread **threads;
	ProcessSymbol *symbols;
	uintptr_t entry;
	void addPage(uintptr_t vaddr, void* paddr, uint8_t flags);
public:
	Process();
	~Process();
	void startup();
	void addThread(Thread *thread, bool suspended);
	
	uint64_t getId();
	
	PPTE pagetable;
	uintptr_t addSection(SectionType type, size_t size);
	void addSymbol(const char *name, uintptr_t ptr);
	void setEntryAddress(uintptr_t ptr);
	
	uintptr_t getSymbolByName(const char* name);
	
	void writeData(uintptr_t address, void* src, size_t size);
	void readData(void* dst, uintptr_t address, size_t size);
	char *readString(uintptr_t address);
	
	uintptr_t getVirtualAddress(void* addr);
	void *getPhysicalAddress(uintptr_t ptr);
};
