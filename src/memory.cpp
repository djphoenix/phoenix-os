#include "pxlib.h"
PGRUB grub_data;
PPTE pagetable = (PPTE)0x10000;
PALLOCTABLE allocs = 0;
void* first_free = (void*)0x10000;
_uint64 last_page = 1;
PPML4E get_page(void* base_addr)
{
	_uint64 i = (_uint64) base_addr >> 12;
	PDE pde = (PDE)((_uint64)pagetable[(i >> 27) & 0x1FF] & 0xFFFFFFFFFFFFF000);
	if(pde == 0) return 0;
	PDPE pdpe = (PDPE)((_uint64)pde[(i >> 18) & 0x1FF] & 0xFFFFFFFFFFFFF000);
	if(pdpe == 0) return 0;
	PPML4E page = (PPML4E)((_uint64)pdpe[(i >> 9) & 0x1FF] & 0xFFFFFFFFFFFFF000);
	if(page == 0) return 0;
	return &page[i & 0x1FF];
}
void memory_init()
{
	asm("movq $_start, %q0":"=a"(kernel_data.kernel)); kernel_data.stack = 0x1000; kernel_data.stack_top = 0x10000;
	asm("movq $__data_start__, %q0":"=a"(kernel_data.data));
	asm("movq $__rdata_end__, %q0":"=a"(kernel_data.data_top));
	asm("movq $__bss_start__, %q0":"=a"(kernel_data.bss));
	asm("movq $__bss_end__, %q0":"=a"(kernel_data.bss_top));
	asm("movq $__modules_start__, %q0":"=a"(kernel_data.modules));
	asm("movq $__modules_end__, %q0":"=a"(kernel_data.modules_top));
	// Buffering BIOS interrupts
	for(int i=0; i<256; i++){
		PINTERRUPT32 intr = (PINTERRUPT32)(((_uint64)i & 0xFF)*sizeof(INTERRUPT32));
		interrupts32[i] = *intr;
	}
	// Buffering grub data
	kernel_data.flags = grub_data->flags;
	kernel_data.mem_lower = grub_data->mem_lower;
	kernel_data.mem_upper = grub_data->mem_upper;
	kernel_data.boot_device = grub_data->boot_device;
	kernel_data.boot_device = grub_data->boot_device;
	kernel_data.mods = (PMODULE)((_uint64)grub_data->pmods_addr & 0xFFFFFFFF);
	if((_uint64)kernel_data.mods < 0x100000)
		kernel_data.mods = (PMODULE)((_uint64)kernel_data.mods + 0x100000);
	kernel_data.mmap_length = grub_data->mmap_length;
	kernel_data.mmap_addr = (void*)((_uint64)grub_data->pmmap_addr & 0xFFFFFFFF);
	if((_uint64)kernel_data.mmap_addr < 0x100000)
		kernel_data.mmap_addr = (void*)((_uint64)kernel_data.mmap_addr + 0x100000);
	
	char cmdline[256], bl_name[256]; long cmdlinel = 0, blnamel = 0;
	if(((kernel_data.flags & 4) == 4) && (grub_data->pcmdline != 0)){
		char* c = (char*)((_uint64)grub_data->pcmdline & 0xFFFFFFFF);
		if((_uint64)c < 0x100000)
			c = (char*)((_uint64)c + 0x100000);
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

			(*(_uint64*)(get_page((void*)((_uint64)modules[i].start & 0xFFFFF000)))) &= ~4; // Set module pages as system
			for(_uint64 addr = modules[i].start & 0xFFFFF000; addr < modules[i].end & 0xFFFFF000; addr += 0x1000)
				(*(_uint64*)(get_page((void*)addr))) &= ~4;

			c = (PGRUBMODULE)((_uint64)c + 16);
		}
	}

	// Initialization of pagetables
	(*(_uint64*)(get_page((void*)0x00000000))) &= 0xFFFFFFFFFFFFFFF0; // BIOS Data
	for(_uint64 addr = 0x0A0000; addr < 0x0C8000; addr += 0x1000) // Video data & VGA BIOS
		(*(_uint64*)(get_page((void*)addr))) &= ~4;
	for(_uint64 addr = 0x0C8000; addr < 0x0EF000; addr += 0x1000) // Reserved for many systems
		(*(_uint64*)(get_page((void*)addr))) &= ~4;
	for(_uint64 addr = 0x0F0000; addr < 0x100000; addr += 0x1000) // BIOS Code
		(*(_uint64*)(get_page((void*)addr))) &= ~4;
	for(_uint64 addr = kernel_data.stack; addr < (kernel_data.stack_top & 0xFFFFFFFFFFFFF000) + 0x1000; addr += 0x1000) // PXOS Stack
		(*(_uint64*)(get_page((void*)addr))) &= ~4;
	for(_uint64 addr = kernel_data.kernel; addr < (kernel_data.data_top & 0xFFFFFFFFFFFFF000) + 0x1000; addr += 0x1000) // PXOS Code & Data
		(*(_uint64*)(get_page((void*)addr))) &= ~4;
	for(_uint64 addr = kernel_data.modules; addr < (kernel_data.modules_top & 0xFFFFFFFFFFFFF000) + 0x1000; addr += 0x1000) // PXOS Modules
		(*(_uint64*)(get_page((void*)addr))) &= ~4;
	for(_uint64 addr = kernel_data.bss; addr < (kernel_data.bss_top & 0xFFFFFFFFFFFFF000) + 0x1000; addr += 0x1000) // PXOS BSS
		(*(_uint64*)(get_page((void*)addr))) &= ~4;

	(*(_uint64*)(get_page((void*)pagetable))) &= ~4; // Setting pagetable pages as system
	for(short i = 0; i < 512; i++) {
		PPDE pde = pagetable[i];
		if(((_uint64)pde & 1) == 0) continue;
		pde = (PPDE)((_uint64)pde & 0xFFFFFFFFFFFFF000);
		(*(_uint64*)(get_page((void*)pde))) &= ~4;
		for(short j = 0; j < 512; j++) {
			PPDPE pdpe = pde[j];
			if(((_uint64)pdpe & 1) == 0) continue;
			pdpe = (PPDPE)((_uint64)pdpe & 0xFFFFFFFFFFFFF000);
			(*(_uint64*)(get_page((void*)pdpe))) &= ~4;
			for(short k = 0; k < 512; k++) {
				PPML4E pml4e = pdpe[k];
				if(((_uint64)pml4e & 1) == 0) continue;
				pml4e = (PPML4E)((_uint64)pml4e & 0xFFFFFFFFFFFFF000);
				(*(_uint64*)(get_page((void*)pml4e))) &= ~4;
			}
		}
	}
	// Clearing unused pages
	for(short i = 0; i < 512; i++) {
		PPDE pde = pagetable[i];
		if(((_uint64)pde & 1) == 0) continue;
		pde = (PPDE)((_uint64)pde & 0xFFFFFFFFFFFFF000);
		for(short j = 0; j < 512; j++) {
			PPDPE pdpe = pde[j];
			if(((_uint64)pdpe & 1) == 0) continue;
			pdpe = (PPDPE)((_uint64)pdpe & 0xFFFFFFFFFFFFF000);
			for(short k = 0; k < 512; k++) {
				PPML4E pml4e = pdpe[k];
				if(((_uint64)pml4e & 1) == 0) continue;
				pml4e = (PPML4E)((_uint64)pml4e & 0xFFFFFFFFFFFFF000);
				for(short l = 0; l < 512; l++) {
					void* addr = pml4e[l];
					if(((_uint64)addr & 4) != 0)
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
			mod->start = (void*)((_uint64)modules[i].start & 0xFFFFFFFF);
			mod->end = (void*)((_uint64)modules[i].end & 0xFFFFFFFF);
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
		if(p != 0) c = (*(_uint64*)(p)) & 0xF;
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
	_uint64 i=last_page-1;
	while(i<0xFFFFFFFFFF000){
		i++;
		addr = (void*)(i << 12);
		page = get_page(addr);
		if((page == 0) || (*(_uint64*)page & 1) == 0) break;
	}
	last_page = i;
	PDE pde = (PDE)((_uint64)pagetable[(i >> 27) & 0x1FF] & 0xFFFFFFFFFFFFF000);
	PDPE pdpe = (PDPE)((_uint64)pde[(i >> 18) & 0x1FF] & 0xFFFFFFFFFFFFF000);
	PDPE pdpen = (PDPE)((_uint64)pde[((i+1) >> 18) & 0x1FF] & 0xFFFFFFFFFFFFF000);
	page = (PPML4E)((_uint64)pdpe[(i >> 9) & 0x1FF] & 0xFFFFFFFFFFFFF000);
	if(((_uint64)pde[((i+2) >> 18) & 0x1FF] & 1) == 0){
		page[i & 0x1FF] = (void*)((_uint64)addr | 3);
		i++; i++;
		pde[(i >> 18) & 0x1FF] = (void**)((_uint64)addr | 3);
		for(short j=0; j<0x200; j++)
			((_uint64*)addr)[j] = 0;
		return palloc(sys);
	}
	if(((_uint64)pdpen[((i+1) >> 9) & 0x1FF] & 1) == 0){
		page[i & 0x1FF] = (void*)((_uint64)addr | 3);
		i++;
		pdpen[(i >> 9) & 0x1FF] = (void*)((_uint64)addr | 3);
		for(short j=0; j<0x200; j++)
			((_uint64*)addr)[j] = ((_uint64)addr + (j+1)*0x1000);
		return palloc(sys);
	}
	page[i & 0x1FF] = (void*)(((_uint64)addr) | (sys == 0 ? 7 : 3));
	for(i=0; i<0x200; i++)
		((_uint64*)addr)[i] = 0;
	return addr;
}
void pfree(void* page){
	PPML4E pdata = get_page(page);
	if((pdata != 0) && ((*(_uint64*)pdata & 5) == 1)){
		*(_uint64*)pdata &= 0xFFFFFFFFFFFFFFF0;
		if(((_uint64)page >> 12) < last_page) last_page = (_uint64)page >> 12;
	}
}
void* malloc(_uint64 size, int align)
{
	if(size == 0) return (void*)0;
	_uint64 ns = (_uint64)first_free, ne; char f;
	PALLOCTABLE t;
	while(1){
		if(ns%align != 0) ns = ns + align - (ns%align);
		ne = ns + size;
		f=0;
		_uint64 ps = ns >> 12, pe = (ne >> 12) + (((ne & 0xFFF) !=0) ? 1 : 0);
		for(_uint64 i=ps; i < pe; i++){
			PPML4E pdata = get_page((void*)((_uint64)i << 12));
			if((pdata!=0)&&((*(_uint64*)pdata & 1)==1)&&((*(_uint64*)pdata & 4)==0)){
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
		for(_uint64 i=ps; i < pe; i++){
			PML4E page = get_page((void*)(i*0x1000));
			if((page == 0)||(*(_uint64*)page & 1) == 0) {
				void *t = palloc(0);
				if((_uint64)t != (i * 0x1000)) {
					f=1;
					break;
				}
			}
		}
		if(f==0) break;
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
			if(t->next == 0) {
				t->next = (PALLOCTABLE)palloc(1);
				((PALLOCTABLE)t->next)->next = 0;
			}
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