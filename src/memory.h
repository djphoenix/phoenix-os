#ifndef MEMORY_H
#define MEMORY_H
#include "pxlib.h"
#include "interrupts.h"
typedef void* PML4E, **PPML4E;
typedef PPML4E PDPE, *PPDPE;
typedef PPDPE PDE, *PPDE;
typedef PPDE PTE, *PPTE;
typedef struct {
	long start;
	long end;
} GRUBMODULE, *PGRUBMODULE;

typedef struct {
	long flags;
	long mem_lower, mem_upper;
	long boot_device;
	long pcmdline;
	long mods_count; long pmods_addr;
	long syms[3];
	long mmap_length; long pmmap_addr;
	long drives_length; long pdrives_addr;
	long pconfig_table;
	long pboot_loader_name;
	long papm_table;
	long pvbe_control_info, pvbe_mode_info, pvbe_mode, pvbe_interface_seg, pvbe_interface_off, pvbe_interface_len;
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
extern void* palloc(char sys = 0);
extern void* malloc(_int64 size, int align = 4);
extern void mfree(void* addr);
#endif
