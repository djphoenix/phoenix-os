#include "pxlib.h"
PGRUB grub_data;
PPTE pagetable = (PPTE)0x1000;
PPML4E get_page(void* base_addr){
	base_addr = (void*)((_int64)base_addr & 0xFFFFFFFFFFFFF000);
	for(short i = 0; i < 512; i++){
		PPDE pde = pagetable[i];
		if(((_int64)pde & 1) == 0) continue;
		pde = (PPDE)((_int64)pde & 0xFFFFFFFFFFFFF000);
		for(short j = 0; j < 512; j++){
			PPDPE pdpe = pde[j];
			if(((_int64)pdpe & 1) == 0) continue;
			pdpe = (PPDPE)((_int64)pdpe & 0xFFFFFFFFFFFFF000);
			for(short k = 0; k < 512; k++){
				PPML4E pml4e = pdpe[k];
				if(((_int64)pml4e & 1) == 0) continue;
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
	(*(_int64*)(get_page((void*)0x00000000))) &= 0xFFFFFFFFFFFFFFF0; // BIOS Data
	for(_int64 addr = 0x0A0000; addr < 0x0C8000; addr += 0x1000){ // Video data & VGA BIOS
		(*(_int64*)(get_page((void*)addr))) &= ~4;
	}
	for(_int64 addr = 0x0F0000; addr < 0x100000; addr += 0x1000){ // BIOS Code
		(*(_int64*)(get_page((void*)addr))) &= ~4;
	}
	for(_int64 addr = kernel; addr < data_top; addr += 0x1000){ // PXOS Code & Data
		(*(_int64*)(get_page((void*)addr))) &= ~4;
	}
	for(_int64 addr = bss; addr < bss_top; addr += 0x1000){ // PXOS BSS
		(*(_int64*)(get_page((void*)addr))) &= ~4;
	}
	
	(*(_int64*)(get_page((void*)pagetable))) &= ~4; // Setting pagetable pages as system
	for(short i = 0; i < 512; i++){
		PPDE pde = pagetable[i];
		if(((_int64)pde & 1) == 0) continue;
		pde = (PPDE)((_int64)pde & 0xFFFFFFFFFFFFF000);
		(*(_int64*)(get_page((void*)pde))) &= ~4;
		for(short j = 0; j < 512; j++){
			PPDPE pdpe = pde[j];
			if(((_int64)pdpe & 1) == 0) continue;
			pdpe = (PPDPE)((_int64)pdpe & 0xFFFFFFFFFFFFF000);
			(*(_int64*)(get_page((void*)pdpe))) &= ~4;
			for(short k = 0; k < 512; k++){
				PPML4E pml4e = pdpe[k];
				if(((_int64)pml4e & 1) == 0) continue;
				pml4e = (PPML4E)((_int64)pml4e & 0xFFFFFFFFFFFFF000);
				(*(_int64*)(get_page((void*)pml4e))) &= ~4;
			}
		}
	}
	for(short i = 0; i < 512; i++){ // Clearing unused pages
		PPDE pde = pagetable[i];
		if(((_int64)pde & 1) == 0) continue;
		pde = (PPDE)((_int64)pde & 0xFFFFFFFFFFFFF000);
		for(short j = 0; j < 512; j++){
			PPDPE pdpe = pde[j];
			if(((_int64)pdpe & 1) == 0) continue;
			pdpe = (PPDPE)((_int64)pdpe & 0xFFFFFFFFFFFFF000);
			for(short k = 0; k < 512; k++){
				PPML4E pml4e = pdpe[k];
				if(((_int64)pml4e & 1) == 0) continue;
				pml4e = (PPML4E)((_int64)pml4e & 0xFFFFFFFFFFFFF000);
				for(short l = 0; l < 512; l++){
					void* addr = pml4e[l];
					if(((_int64)addr & 4) != 0){
						pml4e[l] = 0;
					}
				}
			}
		}
	}
}
void memmap(){
	char *m = (char*)0xB8000;
	int i;
	for(i = 0; i < 0x7D0000; i+=0x1000){
		void *p = get_page((void*)i);
		char c = 0, cl;
		if(p != 0) c = (*(_int64*)(p)) & 0xF;
		cl = 15;
		if(c & 1){
			if(c & 4){
				c = 'U'; cl = 0xA0;
			} else {
				c = 'S'; cl = 0xC0;
			}
		} else {
			c = 'E'; cl = 7;
		}
		*m = c;
		m++;
		*m = cl;
		m++;
	}
}
void* malloc(_int64 size, int align)
{
	return (void*)size;
}
