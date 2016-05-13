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

uint64_t countval = 0;
char *usercode = 0;
Mutex codemutex = Mutex();

void process_loop() {
	for(;;) asm("hlt");
}

ProcessManager* ProcessManager::manager = 0;
Mutex managerMutex = Mutex();
ProcessManager* ProcessManager::getManager() {
	asm volatile("pushfq; cli");
	managerMutex.lock();
	if (!manager) manager = new ProcessManager();
	managerMutex.release();
	asm volatile("popfq");
	return manager;
}
ProcessManager::ProcessManager() {
	Interrupts::addCallback(0x20, &ProcessManager::TimerHandler);
	processSwitchMutex = Mutex();
	processes = 0;
}
bool ProcessManager::TimerHandler(intcb_regs *regs) {
	return getManager()->SwitchProcess(regs);
}
bool ProcessManager::SwitchProcess(intcb_regs *regs) {
	processSwitchMutex.lock();
	if (countval++ % 0x1001 != 0) {
		processSwitchMutex.release();
		return false;
	}
	printf("%hhx - %hx - %llx: %08x -> %08llx\n",
		   regs->dpl, regs->cs, regs->rip,
		   regs->cpuid, countval);
	processSwitchMutex.release();
	return true;
}

uint64_t ProcessManager::RegisterProcess(Process *process) {
	processSwitchMutex.lock();
	uint64_t pcount = 0, pid = 1;
	while (processes != 0 && processes[pcount] != 0) {
		pid = MAX(pid, processes[pcount]->getId()+1);
		pcount++;
	}
	Process **old = processes;
	processes = (Process**)Memory::alloc(sizeof(Process*)*(pcount+2));
	Memory::copy(processes, old, sizeof(Process*)*pcount);
	Memory::free(old);
	processes[pcount+0] = process;
	processes[pcount+1] = 0;
	processSwitchMutex.release();
	return pid;
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
}

Process::Process() {
	id = -1;
	pagetable = 0;
	symbols = 0;
	threads = 0;
}
Process::~Process() {
	if (pagetable != 0) {
		PTE addr;
		for (uint16_t ptx = 0; ptx < 512; ptx++) {
			addr = pagetable[ptx];
			if (!addr.present) continue;
			PPTE ppde = (PPTE)PTE_GET_PTR(addr);
			for (uint16_t pdx = 0; pdx < 512; pdx++) {
				addr = ppde[pdx];
				if (!addr.present) continue;
				PPTE pppde = (PPTE)PTE_GET_PTR(addr);
				for (uint16_t pdpx = 0; pdpx < 512; pdpx++) {
					addr = pppde[pdpx];
					if (!addr.present) continue;
					PPTE ppml4e = (PPTE)PTE_GET_PTR(addr);
					for (uint16_t pml4x = 0; pml4x < 512; pml4x++) {
						addr = ppml4e[pml4x];
						if (!addr.present) continue;
						void *page = PTE_GET_PTR(addr);
						if ((uintptr_t)page ==
							(((uintptr_t)ptx << (12+9+9+9)) |
							 ((uintptr_t)pdx << (12+9+9)) |
							 ((uintptr_t)pdpx << (12+9)) |
							 ((uintptr_t)pml4x << (12)))) continue;
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
			// TODO: unschedule thread
			delete threads[tid];
			tid++;
		}
	}
	Memory::free(threads);
}

uint64_t Process::getId() { return id; }

void Process::addPage(uintptr_t vaddr, void* paddr, uint8_t flags) {
	uint16_t ptx   = (vaddr >> (12+9+9+9)) & 0x1FF;
	uint16_t pdx   = (vaddr >> (12+9+9))   & 0x1FF;
	uint16_t pdpx  = (vaddr >> (12+9))     & 0x1FF;
	uint16_t pml4x = (vaddr >> (12))       & 0x1FF;
	if (pagetable == 0) {
		pagetable = (PPTE)Memory::palloc();
		addPage((uintptr_t)pagetable, pagetable, 0);
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
	if (size == 0) return 0;
	size_t pages = (size >> 12) +1;
	uintptr_t vaddr = 0xF000000000;
	if (type == SectionTypeStack)
		vaddr = 0xA000000000;
check:
	for (uintptr_t caddr = vaddr; caddr < vaddr + size; caddr += 0x1000) {
		if (getPhysicalAddress(vaddr) != 0) {
			vaddr += 0x1000; goto check;
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
		while (old[symbolcount].name != 0 &&
			   old[symbolcount].ptr != 0)
			symbolcount++;
	}
	symbols = (ProcessSymbol*)Memory::alloc(sizeof(ProcessSymbol)*(symbolcount+2));
	Memory::copy(symbols, old, sizeof(ProcessSymbol)*symbolcount);
	Memory::free(old);
	symbols[symbolcount].ptr = ptr;
	size_t namelen = strlen(name);
	symbols[symbolcount].name = (char*)Memory::alloc(namelen+1);
	Memory::copy(symbols[symbolcount].name, (void*)name, namelen);
	symbols[symbolcount].name[namelen] = 0;
	symbols[symbolcount+1].ptr = 0;
	symbols[symbolcount+1].name = 0;
}
void Process::setEntryAddress(uintptr_t ptr) {
	entry = ptr;
}
uintptr_t Process::getSymbolByName(const char* name) {
	if (symbols == 0) return 0;
	size_t idx = 0;
	while (symbols[idx].ptr != 0 && symbols[idx].name != 0) {
		if (strcmp(symbols[idx].name, (char*)name) == 1)
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
	if (length == 0) return 0;
	char *buf = (char*)Memory::alloc(length+1);
	readData(buf, address, length+1);
	return buf;
}
uintptr_t Process::getVirtualAddress(void* addr) {
	// TODO: make this work
	return 0;
}
void *Process::getPhysicalAddress(uintptr_t ptr) {
	if (pagetable == 0) return 0;
	uint16_t ptx   = (ptr >> (12+9+9+9)) & 0x1FF;
	uint16_t pdx   = (ptr >> (12+9+9))   & 0x1FF;
	uint16_t pdpx  = (ptr >> (12+9))     & 0x1FF;
	uint16_t pml4x = (ptr >> (12))       & 0x1FF;
	uintptr_t off = ptr & 0xFFF;
	PTE addr;
	addr = pagetable[ptx];
	PPTE pde = addr.present ? (PPTE)PTE_GET_PTR(addr) : 0;
	if(pde == 0) return 0;
	addr = pde[pdx];
	PPTE pdpe = addr.present ? (PPTE)PTE_GET_PTR(addr) : 0;
	if(pdpe == 0) return 0;
	addr = pdpe[pdpx];
	PPTE page = addr.present ? (PPTE)PTE_GET_PTR(addr) : 0;
	if(page == 0) return 0;
	addr = page[pml4x];
	void *_ptr = addr.present ? (void*)((uintptr_t)PTE_GET_PTR(addr) + off): 0;
	return _ptr;
}
void Process::addThread(Thread *thread, bool suspended) {
	if (thread->regs.rsp == 0) {
		thread->regs.rsp = addSection(SectionTypeStack, 0x7FFF) + 0x8000;
	}
	thread->suspend_ticks = suspended ? -1 : 0;
	
	uint64_t tcount = 0;
	while (threads != 0 && threads[tcount] != 0) tcount++;
	Thread **old = threads;
	threads = (Thread**)Memory::alloc(sizeof(Thread*)*(tcount+2));
	Memory::copy(threads, old, sizeof(Thread*)*tcount);
	Memory::free(old);
	threads[tcount+0] = thread;
	threads[tcount+1] = 0;
	// TODO: Schedule thread
}
void Process::startup() {
	Thread *thread = new Thread();
	thread->regs.rip = entry;
	thread->regs.rflags = 0x200;
	id = (ProcessManager::getManager())->RegisterProcess(this);
	addThread(thread, false);
}
