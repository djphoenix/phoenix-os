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

#pragma once
#include "pxlib.hpp"
#include "interrupts.hpp"
#include "multiboot_info.hpp"
typedef void* PML4E, **PPML4E;
typedef PPML4E PDPE, *PPDPE;
typedef PPDPE PDE, *PPDE;
typedef PPDE PTE, *PPTE;
typedef struct {
	uint32_t start;
	uint32_t end;
} GRUBMODULE, *PGRUBMODULE;

typedef struct {
	uint32_t flags;
	uint32_t mem_lower, mem_upper;
	uint32_t boot_device;
	uint32_t pcmdline;
	uint32_t mods_count; long pmods_addr;
	uint32_t syms[3];
	uint32_t mmap_length; long pmmap_addr;
	uint32_t drives_length; long pdrives_addr;
	uint32_t pconfig_table;
	uint32_t pboot_loader_name;
	uint32_t papm_table;
	uint64_t pvbe_control_info, pvbe_mode_info, pvbe_mode, pvbe_interface_seg, pvbe_interface_off, pvbe_interface_len;
} GRUB, *PGRUB;
typedef struct {
	void* addr;
	size_t size;
} ALLOC, *PALLOC;
typedef struct {
	ALLOC allocs[255];
	void* next;
	int64_t reserved;
} ALLOCTABLE, *PALLOCTABLE;
extern PGRUB grub_data;

class Memory {
	static PPTE pagetable;
	static PALLOCTABLE allocs;
	static void* first_free;
	static _uint64 last_page;
	static GRUBMODULE modules[256];
	static PPML4E get_page(void* base_addr);
	static Mutex page_mutex, heap_mutex;
	static void* _palloc(bool sys = false);
public:
	static void map();
	static void init();
	static void* salloc(void* mem);
	static void* palloc(bool sys = false);
	static void* alloc(size_t size, size_t align = 4);
	static void pfree(void* page);
	static void free(void* addr);
	static void copy(void* dest, void* src, size_t count);
};

void __inline MmioWrite32(void *p, uint32_t data) { Memory::salloc(p); *(volatile int *)(p) = data; }
uint32_t __inline MmioRead32(void *p) { Memory::salloc(p); return *(volatile int *)(p); }
