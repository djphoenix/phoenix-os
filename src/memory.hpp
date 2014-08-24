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
extern void memmap();
extern void memory_init();
extern void* salloc(void* mem);
extern void* palloc(char sys = 0);
extern void* malloc(_uint64 size, int align = 4);
extern void mfree(void* addr);
extern void memcpy(char *dest, char *src, _uint64 count);
#endif
