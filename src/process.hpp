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
	size_t offset;
	size_t vaddr;
	size_t size;
	size_t fsize;
} PROCSECT;
typedef struct {
	size_t offset;
	size_t type;
	size_t sym;
	size_t add;
	size_t sect;
} PROCREL;
typedef struct {
	size_t offset;
	uint32_t type;
	uint32_t sect;
	char* name;
} PROCSYM;
typedef struct {
	size_t seg_cnt;
	PROCSECT* segments;
	size_t sect_cnt;
	PROCSECT* sections;
	size_t reloc_cnt;
	PROCREL* relocs;
	size_t sym_cnt;
	PROCSYM* symbols;
	uint64_t entry_sym;
} PROCSTARTINFO, *PPROCSTARTINFO;
class Process;
class ProcessManager {
private:
	Process** processes;
	static Mutex* processSwitchMutex;
	static ProcessManager* manager;
	static void SwitchProcess();
public:
	_uint64 RegisterProcess(Process* process);
	static ProcessManager* getManager();
};

class Process {
private:
	uint64_t id;
	PPTE pagetable;
	PROCSTARTINFO psinfo;
	struct {
		uint64_t rax, rcx, rdx, rbx;
		uint64_t rsi, rdi, rbp, rsp;
		uint64_t rip, rflags;
		uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
	} regs;
	bool suspend;
public:
	Process(PROCSTARTINFO psinfo);
};
