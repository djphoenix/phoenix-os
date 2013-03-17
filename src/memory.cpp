#include "pxlib.h"
PGRUB grub_data;
PPTE pagetable = (PPTE)0x1000;
void l2h(const _int64 i, char* s){
	for(int c = 0; c < sizeof(i)/2; c++){
		char b = (i >> ((sizeof(i)-c-1)*8)) & 0xFF;
		s[c*2+1] = ((b & 0xF) > 9) ? ('A' + (b & 0xF) - 10) : ('0' + (b & 0xF));
		b >>= 4;
		s[c*2  ] = ((b & 0xF) > 9) ? ('A' + (b & 0xF) - 10) : ('0' + (b & 0xF));
	}
	for(int c = sizeof(i)/2; c < sizeof(i); c++){
		char b = (i >> ((sizeof(i)-c-1)*8)) & 0xFF;
		s[c*2+2] = ((b & 0xF) > 9) ? ('A' + (b & 0xF) - 10) : ('0' + (b & 0xF));
		b >>= 4;
		s[c*2+1] = ((b & 0xF) > 9) ? ('A' + (b & 0xF) - 10) : ('0' + (b & 0xF));
	}
}
PPML4E get_page(void* base_addr){
	base_addr = (void*)((_int64)base_addr & 0xFFFFFFFFFFFFF000);
	for(short i = 0; i < 512; i++){
		PPDE pde = pagetable[i];
		if((_int64)pde & 1 == 0) continue;
		pde = (PPDE)((_int64)pde & 0xFFFFFFFFFFFFF000);
		for(short j = 0; j < 512; j++){
			PPDPE pdpe = pde[j];
			if((_int64)pdpe & 1 == 0) continue;
			pdpe = (PPDPE)((_int64)pdpe & 0xFFFFFFFFFFFFF000);
			for(short k = 0; k < 512; k++){
				PPML4E pml4e = pdpe[k];
				if((_int64)pml4e & 1 == 0) continue;
				pml4e = (PPML4E)((_int64)pml4e & 0xFFFFFFFFFFFFF000);
				for(short l = 0; l < 512; l++){
					void* addr = pml4e[l];
					addr = (void*)((_int64)addr & 0xFFFFFFFFFFFFF000);
					if(addr == base_addr) return &pml4e[l];
				}
			}
		}
	}
	return (PPML4E)0;
}
void memory_init()
{
	print("Initializing memory...\n");
	_int64 kernel = 0, stack = 0, data = 0, data_top = 0, bss = 0, bss_top = 0;
	asm("movq $_start, %q0":"=a"(kernel));
	asm("movq $__data_start__, %q0":"=a"(data));
	asm("movq $__rt_psrelocs_end, %q0":"=a"(data_top)); data_top = (data_top & 0xFFFFF000) + 0x1000;
	asm("movq $__bss_start__, %q0":"=a"(bss));
	asm("movq $__bss_end__, %q0":"=a"(bss_top)); bss_top = (bss_top & 0xFFFFF000) + 0x1000;
	
	// Initialization of pagetables
	PPML4E pml4e;
	pml4e = get_page((void*)0x00000000); (*(_int64*)(pml4e)) |= 4; // BIOS Data
	for(_int64 addr = 0x0A0000; addr < 0x0C8000; addr += 0x1000){ // Video data & VGA BIOS
		pml4e = get_page((void*)addr); (*(_int64*)(pml4e)) |= 4;
	}
	for(_int64 addr = 0x0F0000; addr < 0x100000; addr += 0x1000){ // BIOS Code
		pml4e = get_page((void*)addr); (*(_int64*)(pml4e)) |= 4;
	}
	for(_int64 addr = kernel; addr < data_top; addr += 0x1000){ // PXOS Code & Data
		pml4e = get_page((void*)addr); (*(_int64*)(pml4e)) |= 4;
	}
	for(_int64 addr = bss; addr < bss_top; addr += 0x1000){ // PXOS BSS
		pml4e = get_page((void*)addr); (*(_int64*)(pml4e)) |= 4;
	}
}
