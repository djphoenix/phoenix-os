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
