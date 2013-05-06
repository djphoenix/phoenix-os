#include "multiboot_info.h"

void kernel_info()
{
	_int64 kernel = 0, stack = 0, stack_top = 0, data = 0, data_top = 0, bss = 0, bss_top = 0;
	asm("movq $_start, %q0":"=a"(kernel)); stack = kernel - 0x10000; stack_top = kernel;
	asm("movq $__data_start__, %q0":"=a"(data));
	asm("movq $__rt_psrelocs_end, %q0":"=a"(data_top)); data_top = (data_top & 0xFFFFF000) + 0x1000;
	asm("movq $__bss_start__, %q0":"=a"(bss));
	asm("movq $__bss_end__, %q0":"=a"(bss_top)); bss_top = (bss_top & 0xFFFFF000) + 0x1000;

	print(" - Kernel info:\n");
	print("Kernel:\t"); printq(kernel); print("\n");
	print("Stack:\t"); printq(stack); print(" - "); printq(stack_top-1); print("\n");
	print("Data:\t"); printq(data); print(" - "); printq(data_top-1); print("\n");
	print("BSS:\t"); printq(bss); print(" - "); printq(bss_top-1); print("\n");
	print(" - Grub data:\n");
	print("Flags:\t"); printl(grub_data->flags); print("\n");
	if((grub_data->flags & 1) == 1) {
		print("Mem:\t"); printl(grub_data->mem_lower); print(" - "); printl(grub_data->mem_upper); print("\n");
	}
	if((grub_data->flags & 2) == 2) {
		print("Boot device:\t"); printl(grub_data->boot_device); print("\n");
	}
	if(((grub_data->flags & 4) == 4) && (grub_data->pcmdline != 0)) {
		print("Command line:\t"); print((char*)((_int64)grub_data->pcmdline & 0xFFFFFFFF)); print("\n");
	}
	if(((grub_data->flags & 512) == 512) && (grub_data->pboot_loader_name != 0)) {
		print("Loader name:\t"); print((char*)((_int64)grub_data->pboot_loader_name & 0xFFFFFFFF)); print("\n");
	}
}