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

#ifndef PROCESS_H
#define PROCESS_H
#include "pxlib.hpp"
#include "memory.hpp"
extern void process_loop();
typedef struct {
    _uint64 offset;
    _uint64 vaddr;
    _uint64 size;
    _uint64 fsize;
} PROCSECT;
typedef struct {
    _uint64 offset;
    uint type;
    uint sym;
    _uint64 add;
    _uint64 sect;
} PROCREL;
typedef struct {
    _uint64 offset;
    uint type;
    uint sect;
    char* name;
} PROCSYM;
typedef struct {
    _uint64 seg_cnt;
    PROCSECT* segments;
    _uint64 sect_cnt;
    PROCSECT* sections;
    _uint64 reloc_cnt;
    PROCREL* relocs;
    _uint64 sym_cnt;
    PROCSYM* symbols;
    _uint64 entry_sym;
} PROCSTARTINFO, *PPROCSTARTINFO;

class Process {
private:
    PPTE pagetable;
    PROCSTARTINFO psinfo;
    struct {
        _uint64 rax, rcx, rdx, rbx;
        _uint64 rsi, rdi, rbp, rsp;
        _uint64 rip, rflags;
        _uint64 r8, r9, r10, r11, r12, r13, r14, r15;
    } regs;
    bool suspend;
public:
    Process(PROCSTARTINFO psinfo);
};
#endif
