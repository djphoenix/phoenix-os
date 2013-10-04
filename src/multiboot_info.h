//    PhoeniX OS Multiboot subsystem
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

#ifndef MULTIBOOT_INFO_H
#define MULTIBOOT_INFO_H
#include "pxlib.h"
#include "memory.h"
typedef struct {
	void* start;
	void* end;
	void* next;
} MODULE, *PMODULE;

typedef struct {
	long flags;
	long mem_lower, mem_upper;
	long boot_device;
	char* cmdline;
	PMODULE mods;
	long mmap_length; void *mmap_addr;
	char* boot_loader_name;
	_uint64 kernel, stack, stack_top, data, data_top, bss, bss_top, modules, modules_top;
} GRUBDATA, *PGRUBDATA;

extern GRUBDATA kernel_data;
#endif
