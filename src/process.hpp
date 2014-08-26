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
} PROCREL;
typedef struct {
    _uint64 offset;
    char* name;
} PROCSYM;
typedef struct {
    _uint64 entry;
    _uint64 seg_cnt;
    PROCSECT* segments;
    _uint64 reloc_cnt;
    PROCREL* relocs;
    _uint64 sym_cnt;
    PROCSYM* symbols;
    _uint64 link_cnt;
    PROCSYM* links;
} PROCSTARTINFO, *PPROCSTARTINFO;
#endif
