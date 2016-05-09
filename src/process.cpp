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

void process_loop() {
	char* usercode = (char*)Memory::palloc(0);
	usercode[0] = 0xEB;  // jmp
	usercode[1] = 0xFE;  // -2
	
	char* userstack = (char*)Memory::palloc(0)+0x1000;
	
	asm volatile("mov $0x23, %%ax;"
				 "mov %%ax, %%ds;"
				 "mov %%ax, %%es;"
				 "mov %%ax, %%fs;"
				 "mov %%ax, %%gs;"
				 
				 "pushq $0x23;"
				 "pushq %1;"
				 "pushq $0x200;"
				 "pushq $0x1B;"
				 "pushq %0;"
				 "iretq;"
				 ::"c"(usercode), "b"(userstack)
				 );
	for(;;) asm("hlt");
}

Mutex* ProcessManager::processSwitchMutex = 0;
ProcessManager* ProcessManager::manager = 0;
ProcessManager* ProcessManager::getManager() {
	if (manager) return manager;
	processSwitchMutex = new Mutex();
	Interrupts::addCallback(0x20, &ProcessManager::SwitchProcess);
	return manager = new ProcessManager();
}
void ProcessManager::SwitchProcess() {
	processSwitchMutex->lock();
	if (countval++ % 0x1001 != 0) {
		processSwitchMutex->release();
		return;
	}
	printf("%08x -> %08x\n", ACPI::getController()->getCPUID(), countval);
	processSwitchMutex->release();
}

uint64_t ProcessManager::RegisterProcess(Process* process) {
	if (this->processes == 0) {
		this->processes = (Process**)Memory::alloc(sizeof(Process*)*2);
		this->processes[0] = process;
		this->processes[1] = 0;
		return 1;
	} else {
		uint64_t pid = 1;
		while (this->processes[pid-1] != 0) pid++;
		Process** np = (Process**)Memory::alloc(sizeof(Process*)*pid);
		Memory::copy(np, this->processes, sizeof(Process*)*(pid-2));
		Memory::free(this->processes);
		this->processes = np;
		this->processes[pid-1] = process;
		this->processes[pid] = 0;
		return pid;
	}
}

Process::Process(PROCSTARTINFO psinfo) {
	suspend = true;
	this->psinfo = psinfo;
	this->id = (ProcessManager::getManager())->RegisterProcess(this);
	printf("Registered process: %016zx\n", this->id);
}
