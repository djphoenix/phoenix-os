#ifndef MEMORY_H
#define MEMORY_H
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
extern PGRUB grub_data;
extern void memory_init();
#include "pxlib.h"
#endif