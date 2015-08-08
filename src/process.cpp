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

_uint64 countval = 0;

void process_loop()
{
    for(;;) asm("hlt");
}

ProcessManager* ProcessManager::manager = 0;
ProcessManager* ProcessManager::getManager() {
    if (manager) return manager;
    Interrupts::addCallback(0x20,&ProcessManager::SwitchProcess);
    return manager = new ProcessManager();
}
void ProcessManager::SwitchProcess(){
    static bool lock;
    while (lock); lock = 1;
    if (countval++ % 0x101 != 0) {lock = 0; return;}
    printl((ACPI::getController())->getLapicID()); print("->");
    printq(countval); print("\n");
    lock = 0;
}

_uint64 ProcessManager::RegisterProcess(Process* process){
    if (this->processes == 0) {
        this->processes = (Process**)Memory::alloc(sizeof(Process*)*2);
        this->processes[0] = process;
        this->processes[1] = 0;
        return 1;
    } else {
        _uint64 pid = 1;
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

Process::Process(PROCSTARTINFO psinfo){
    suspend=true;
    this->psinfo = psinfo;
    this->id = (ProcessManager::getManager())->RegisterProcess(this);
    print("Registered process: "); printq(this->id); print("\n");
}