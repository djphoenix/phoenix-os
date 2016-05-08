//    PhoeniX OS SMP Subsystem
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

#include "smp.hpp"
#include "process.hpp"

void SMP::startup() {
	ACPI::getController()->activateCPU();
	process_loop();
}

void SMP::init() {
	ACPI* acpi = ACPI::getController();
	uint32_t localId = acpi->getLapicID();
	uint32_t cpuCount = acpi->getCPUCount();
	if (cpuCount == 1) return;
	
	char* smp_init_code = (char*)Memory::palloc(1);
	char smp_init_vector = (((uintptr_t)smp_init_code) >> 12) & 0xFF;
	char* smp_s; asm("movabs $_smp_init,%q0":"=a"(smp_s):);
	char* smp_e; asm("movabs $_smp_end,%q0":"=a"(smp_e):);
	while(smp_s < smp_e) *(smp_init_code++) = *(smp_s++);
	*((uintptr_t*)smp_init_code) = (uintptr_t)acpi->getLapicAddr();
	smp_init_code += 8;
	uintptr_t *stacks = (uintptr_t*)Memory::alloc(sizeof(uintptr_t)*cpuCount);
	uintptr_t *cpuids = (uintptr_t*)Memory::alloc(sizeof(uintptr_t)*cpuCount);
	uint32_t nullcpus = 0;
	for(uint32_t i = 0; i < cpuCount; i++) {
		cpuids[i] = acpi->getLapicIDOfCPU(i);
		if(cpuids[i] != localId)
			stacks[i] = ((uintptr_t)Memory::palloc(1))+0x1000;
		else
			nullcpus++;
	}
	if (nullcpus > 0) nullcpus--;
	*((uintptr_t*)smp_init_code) = (uintptr_t)cpuids; smp_init_code += 8;
	*((uintptr_t*)smp_init_code) = (uintptr_t)stacks; smp_init_code += 8;
	*((uintptr_t*)smp_init_code) = (uintptr_t)(&SMP::startup);
	
	for(uint32_t i = 0; i < cpuCount; i++)
		if(cpuids[i] != localId) acpi->sendCPUInit(cpuids[i]);
	asm("hlt");
	for(uint32_t i = 0; i < cpuCount; i++)
		if(cpuids[i] != localId) acpi->sendCPUStartup(cpuids[i], smp_init_vector);
	asm("hlt");
	for(uint32_t i = 0; i < cpuCount; i++)
		if(cpuids[i] != localId) acpi->sendCPUStartup(cpuids[i], smp_init_vector);
	while (cpuCount - nullcpus != acpi->getActiveCPUCount()) asm("hlt");
	
	Memory::free(cpuids);
	Memory::free(stacks);
}
