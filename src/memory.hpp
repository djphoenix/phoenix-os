//    PhoeniX OS Memory subsystem
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

#ifndef MEMORY_H
#define MEMORY_H
#include "pxlib.hpp"
#include "interrupts.hpp"
#include "multiboot_info.hpp"
typedef void* PML4E, **PPML4E;
typedef PPML4E PDPE, *PPDPE;
typedef PPDPE PDE, *PPDE;
typedef PPDE PTE, *PPTE;
typedef struct {
	uint start;
	uint end;
} GRUBMODULE, *PGRUBMODULE;

typedef struct {
	uint flags;
	uint mem_lower, mem_upper;
	uint boot_device;
	uint pcmdline;
	uint mods_count; long pmods_addr;
	uint syms[3];
	uint mmap_length; long pmmap_addr;
	uint drives_length; long pdrives_addr;
	uint pconfig_table;
	uint pboot_loader_name;
	uint papm_table;
	uint pvbe_control_info, pvbe_mode_info, pvbe_mode, pvbe_interface_seg, pvbe_interface_off, pvbe_interface_len;
} GRUB, *PGRUB;
typedef struct {
	void* addr;
	_int64 size;
} ALLOC, *PALLOC;
typedef struct {
	ALLOC allocs[255];
	void* next;
	_int64 reserved;
} ALLOCTABLE, *PALLOCTABLE;
extern PGRUB grub_data;

class Memory {
    static PPTE pagetable;
    static PALLOCTABLE allocs;
    static void* first_free;
    static _uint64 last_page;
    static GRUBMODULE modules[256];
    static PPML4E get_page(void* base_addr);
    static void map();
public:
    static void init();
    static void* salloc(void* mem);
    static void* palloc(char sys = 0);
    static void* alloc(_uint64 size, int align = 4);
    static void pfree(void* page);
    static void free(void* addr);
    static void copy(void* dest, void* src, _uint64 count);
};

#endif
