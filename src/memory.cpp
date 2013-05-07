#include "pxlib.h"
PGRUB grub_data;
PPTE pagetable = (PPTE)0x1000;
PALLOCTABLE allocs = 0;
void* first_free = (void*)0x1000;
PPML4E get_page(void* base_addr)
{
	base_addr = (void*)((_int64)base_addr & 0xFFFFFFFFFFFFF000);
	for(short i = 0; i < 512; i++) {
		PPDE pde = pagetable[i];
		if(((_int64)pde & 1) == 0) continue;
		pde = (PPDE)((_int64)pde & 0xFFFFFFFFFFFFF000);
		for(short j = 0; j < 512; j++) {
			PPDPE pdpe = pde[j];
			if(((_int64)pdpe & 1) == 0) continue;
			pdpe = (PPDPE)((_int64)pdpe & 0xFFFFFFFFFFFFF000);
			for(short k = 0; k < 512; k++) {
				PPML4E pml4e = pdpe[k];
				if(((_int64)pml4e & 1) == 0) continue;
				pml4e = (PPML4E)((_int64)pml4e & 0xFFFFFFFFFFFFF000);
				for(short l = 0; l < 512; l++) {
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
	_int64 kernel = 0, stack = 0, stack_top = 0, data = 0, data_top = 0, bss = 0, bss_top = 0;
	asm("movq $_start, %q0":"=a"(kernel)); stack = kernel - 0x10000; stack_top = kernel;
	asm("movq $__data_start__, %q0":"=a"(data));
	asm("movq $__rt_psrelocs_end, %q0":"=a"(data_top)); data_top = (data_top & 0xFFFFF000) + 0x1000;
	asm("movq $__bss_start__, %q0":"=a"(bss));
	asm("movq $__bss_end__, %q0":"=a"(bss_top)); bss_top = (bss_top & 0xFFFFF000) + 0x1000;
	// Buffering BIOS interrupts
	for(int i=0; i<256; i++){
		PINTERRUPT32 intr = (PINTERRUPT32)(((_int64)i & 0xFF)*sizeof(INTERRUPT32));
		interrupts32[i] = *intr;
	}
	// Buffering grub data
	kernel_data.flags = grub_data->flags;
	kernel_data.mem_lower = grub_data->mem_lower;
	kernel_data.mem_upper = grub_data->mem_upper;
	kernel_data.boot_device = grub_data->boot_device;
	kernel_data.boot_device = grub_data->boot_device;
	kernel_data.mods = (PMODULE)((_int64)grub_data->pmods_addr & 0xFFFFFFFF);
	if((_int64)kernel_data.mods < 0x100000)
		kernel_data.mods = (PMODULE)((_int64)kernel_data.mods + 0x100000);
	kernel_data.mmap_length = grub_data->mmap_length;
	kernel_data.mmap_addr = (void*)((_int64)grub_data->pmmap_addr & 0xFFFFFFFF);
	if((_int64)kernel_data.mmap_addr < 0x100000)
		kernel_data.mmap_addr = (void*)((_int64)kernel_data.mmap_addr + 0x100000);
	
	char cmdline[256], bl_name[256]; long cmdlinel = 0, blnamel = 0;
	if(((kernel_data.flags & 4) == 4) && (grub_data->pcmdline != 0)){
		char* c = (char*)((_int64)grub_data->pcmdline & 0xFFFFFFFF);
		if((_int64)c < 0x100000)
			c = (char*)((_int64)c + 0x100000);
		int i = 0;
		while((c[i] != 0)&&(i<255)) cmdline[i++] = c[i];
		cmdline[i] = 0;
		cmdlinel = i+1;
	}
	GRUBMODULE modules[256];
	modules[grub_data->mods_count].start = 0;
	if(((kernel_data.flags & 8) == 8) && (grub_data->mods_count != 0) && (kernel_data.mods != 0)){
		PGRUBMODULE c = (PGRUBMODULE)kernel_data.mods;
		for(int i = 0; i < grub_data->mods_count; i++){
			modules[i].start = c->start;
			modules[i].end = c->end;

			(*(_int64*)(get_page((void*)((_int64)modules[i].start & 0xFFFFF000)))) &= ~4; // Set module pages as system
			for(_int64 addr = modules[i].start & 0xFFFFF000; addr < modules[i].end & 0xFFFFF000; addr += 0x1000)
				(*(_int64*)(get_page((void*)addr))) &= ~4;

			c = (PGRUBMODULE)((_int64)c + 16);
		}
	}

	// Initialization of pagetables
	(*(_int64*)(get_page((void*)0x00000000))) &= 0xFFFFFFFFFFFFFFF0; // BIOS Data
	for(_int64 addr = 0x0A0000; addr < 0x0C8000; addr += 0x1000) // Video data & VGA BIOS
		(*(_int64*)(get_page((void*)addr))) &= ~4;
	for(_int64 addr = 0x0C8000; addr < 0x0EF000; addr += 0x1000) // Reserved for many systems
		(*(_int64*)(get_page((void*)addr))) &= ~4;
	for(_int64 addr = 0x0F0000; addr < 0x100000; addr += 0x1000) // BIOS Code
		(*(_int64*)(get_page((void*)addr))) &= ~4;
	for(_int64 addr = stack; addr < stack_top; addr += 0x1000) // PXOS Stack
		(*(_int64*)(get_page((void*)addr))) &= ~4;
	for(_int64 addr = kernel; addr < data_top; addr += 0x1000) // PXOS Code & Data
		(*(_int64*)(get_page((void*)addr))) &= ~4;
	for(_int64 addr = bss; addr < bss_top; addr += 0x1000) // PXOS BSS
		(*(_int64*)(get_page((void*)addr))) &= ~4;

	(*(_int64*)(get_page((void*)pagetable))) &= ~4; // Setting pagetable pages as system
	for(short i = 0; i < 512; i++) {
		PPDE pde = pagetable[i];
		if(((_int64)pde & 1) == 0) continue;
		pde = (PPDE)((_int64)pde & 0xFFFFFFFFFFFFF000);
		(*(_int64*)(get_page((void*)pde))) &= ~4;
		for(short j = 0; j < 512; j++) {
			PPDPE pdpe = pde[j];
			if(((_int64)pdpe & 1) == 0) continue;
			pdpe = (PPDPE)((_int64)pdpe & 0xFFFFFFFFFFFFF000);
			(*(_int64*)(get_page((void*)pdpe))) &= ~4;
			for(short k = 0; k < 512; k++) {
				PPML4E pml4e = pdpe[k];
				if(((_int64)pml4e & 1) == 0) continue;
				pml4e = (PPML4E)((_int64)pml4e & 0xFFFFFFFFFFFFF000);
				(*(_int64*)(get_page((void*)pml4e))) &= ~4;
			}
		}
	}
	// Clearing unused pages
	for(short i = 0; i < 512; i++) {
		PPDE pde = pagetable[i];
		if(((_int64)pde & 1) == 0) continue;
		pde = (PPDE)((_int64)pde & 0xFFFFFFFFFFFFF000);
		for(short j = 0; j < 512; j++) {
			PPDPE pdpe = pde[j];
			if(((_int64)pdpe & 1) == 0) continue;
			pdpe = (PPDPE)((_int64)pdpe & 0xFFFFFFFFFFFFF000);
			for(short k = 0; k < 512; k++) {
				PPML4E pml4e = pdpe[k];
				if(((_int64)pml4e & 1) == 0) continue;
				pml4e = (PPML4E)((_int64)pml4e & 0xFFFFFFFFFFFFF000);
				for(short l = 0; l < 512; l++) {
					void* addr = pml4e[l];
					if(((_int64)addr & 4) != 0)
						pml4e[l] = 0;
				}
			}
		}
	}
	if(cmdlinel > 0) {
		kernel_data.cmdline = (char*)malloc(cmdlinel);
		for(int i=0; i<cmdlinel; i++){
			kernel_data.cmdline[i] = cmdline[i];
		}
	} else {
		kernel_data.cmdline = 0;
	}
	if((kernel_data.flags & 8) == 8){
		PMODULE mod = kernel_data.mods = (PMODULE)malloc(sizeof(MODULE));
		int i=0;
		while(modules[i].start != 0){
			mod->start = (void*)((_int64)modules[i].start & 0xFFFFFFFF);
			mod->end = (void*)((_int64)modules[i].end & 0xFFFFFFFF);
			i++;
			if(modules[i].start != 0)
				mod = (PMODULE)(mod->next = (void*)malloc(sizeof(MODULE)));
		}
		mod->next = (void*)0;
	}
}
void memmap()
{
	char *m = (char*)0xB8000;
	int i;
	for(i = 0; i < 0x7D0000; i+=0x1000) {
		void *p = get_page((void*)i);
		char c = 0, cl;
		if(p != 0) c = (*(_int64*)(p)) & 0xF;
		cl = 15;
		if(c & 1) {
			if(c & 4){
				c = 'U'; cl = 0x20;
			} else {
				c = 'S'; cl = 0x40;
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
void* palloc(char sys)
{
	void *addr; PPML4E page;
	_int64 i=0;
	while(i<0xFFFFFFFFFFFFF){
		i++;
		addr = (void*)(i*0x1000);
		page = get_page(addr);
		if((page != 0) && (*(_int64*)page & 1) == 1) {continue;}
		break;
	}
	PDE pde = (PDE)((_int64)pagetable[(i >> 27) & 0x1FF] & 0xFFFFFFFFFFFFF000);
	PDPE pdpe = (PDPE)((_int64)pde[(i >> 18) & 0x1FF] & 0xFFFFFFFFFFFFF000);
	page = (PPML4E)((_int64)pdpe[(i >> 9) & 0x1FF] & 0xFFFFFFFFFFFFF000);
	page[i & 0x1FF] = (void*)((i * 0x1000) | (sys == 0 ? 7 : 3));
	for(i=0; i<0x200; i++)
		((_int64*)addr)[i] = 0;
	return addr;
}
void* malloc(_int64 size, int align)
{
	if(size == 0) return (void*)0;
	_uint64 ns = (_uint64)first_free, ne; char f;
	PALLOCTABLE t;
	while(1){
		if(ns%align != 0) ns = ns + align - (ns%align);
		ne = ns + size;
		f=0;
		int ps = ns >> 12, pe = (ne >> 12) + (((ne & 0xFFF) !=0) ? 1 : 0);
		for(int i=ps; i < pe; i++){
			PPML4E pdata = get_page((void*)((_uint64)i << 12));
			if((pdata!=0)&&((*(_int64*)pdata & 1)==1)&&((*(_int64*)pdata & 4)==0)){
				ns=(i+1) << 12;
				if(ns%align != 0) ns = ns + align - (ns%align);
				ne = ns + size;
				f=1;
			}
		}
		if(f!=0) continue;
		t = allocs;
		if(t == 0) break;
		while(1){
			for(int i=0;i<255;i++){
				if(t->allocs[i].addr == 0) continue;
				_uint64 as = (_uint64)t->allocs[i].addr, ae = as+(_uint64)t->allocs[i].size;
				if(ne < as) continue;
				if(ae < ns) continue;
				if(
					((ns>=as) and (ns<ae)) or	// NA starts in alloc
					((ne>=as) and (ne<ae)) or	// NA ends in alloc
					((ns>=as) and (ne<ae)) or	// NA inside alloc
					((ns<=as) and (ne>ae))		// alloc inside NA
				) {
					ns=ae;
					if(ns%align != 0) ns = ns + align - (ns%align);
					ne = ns + size;
					f=1;
				}
			}
			if(f!=0) break;
			if(t->next == 0) break;
			t = (PALLOCTABLE)t->next;
		}
		if(f==0) break;
	}
	int ps = ns >> 12, pe = (ne >> 12) + (((ne & 0xFFF) !=0) ? 1 : 0);
	for(int i=ps; i < pe; i++){
		PDE pde = (PDE)((_int64)pagetable[(i >> 27) & 0x1FF] & 0xFFFFFFFFFFFFF000);
		if(pde==0) {
			pagetable[(i >> 27) & 0x1FF] = (PPDE)((_uint64)palloc(1) | 3);
			pde = (PDE)((_int64)pagetable[(i >> 27) & 0x1FF] & 0xFFFFFFFFFFFFF000);
		}
		PDPE pdpe = (PDPE)((_int64)pde[(i >> 18) & 0x1FF] & 0xFFFFFFFFFFFFF000);
		if(pdpe==0) {
			pde[(i >> 18) & 0x1FF] = (PDPE)((_uint64)palloc(1) | 3);
			pdpe = (PDPE)((_int64)pde[(i >> 18) & 0x1FF] & 0xFFFFFFFFFFFFF000);
		}
		PPML4E page = (PPML4E)((_int64)pdpe[(i >> 9) & 0x1FF] & 0xFFFFFFFFFFFFF000);
		if(page==0) {
			pdpe[(i >> 9) & 0x1FF] = (PPML4E)((_uint64)palloc(1) | 3);
			page = (PPML4E)((_int64)pdpe[(i >> 9) & 0x1FF] & 0xFFFFFFFFFFFFF000);
		}
		page[i & 0x1FF] = (void*)((i * 0x1000) | 7);
	}
	// Finding memory slot for alloc record
	if(allocs == 0)
		allocs = (PALLOCTABLE)palloc(1);
	t = allocs;
	int ai;
	while(true){
		ai = -1;
		for(int i = 0; i < 255; i++)
			if(t->allocs[i].addr == 0) {ai = i; break;}
		if(ai == -1) {
			if(t->next == 0)
				t->next = (PALLOCTABLE)palloc(1);
			t = (PALLOCTABLE)t->next;
		} else break;
	}
	t->allocs[ai].addr = (void*) ns;
	t->allocs[ai].size = size;
	if(((_uint64)first_free < ns) && (align == 4)){
		first_free = (void*) (ns + size);
	}
	return (void*) ns;
}
void mfree(void* addr)
{
	PALLOCTABLE t = allocs;
	while(true){
		for(int i = 0; i < 255; i++)
			if(t->allocs[i].addr == addr) {
				t->allocs[i].addr = 0;
				t->allocs[i].size = 0;
				if((_uint64)addr < (_uint64)first_free)
					first_free = addr;
				return;
			}
		if(t->next == 0) return;
		t = (PALLOCTABLE)t->next;
	}
}