#ifndef MEMORY_H
#define MEMORY_H
typedef void* PML4E, **PPML4E;
typedef PPML4E PDPE, *PPDPE;
typedef PPDPE PDE, *PPDE;
typedef PPDE PTE, *PPTE;
typedef struct {
	long flags;
	long mem_lower, mem_upper;
	long boot_device;
	char *cmdline;
	long mods_count; void *mods_addr;
	long syms[3];
	long mmap_length; void *mmap_addr;
	long drives_length; void *drives_addr;
	void *config_table;
	char *boot_loader_name;
	void *apm_table;
	void *vbe_control_info, *vbe_mode_info, *vbe_mode, *vbe_interface_seg, *vbe_interface_off, *vbe_interface_len;
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
#include "pxlib.h"
#endif
