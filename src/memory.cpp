//    PhoeniX OS Memory subsystem
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

#include "memory.hpp"
PGRUB grub_data;

PPTE Memory::pagetable = (PPTE)0x20000;
PALLOCTABLE Memory::allocs = 0;
void* Memory::first_free = (void*)0x2000;
uintptr_t Memory::last_page = 1;
Mutex Memory::page_mutex = Mutex();
Mutex Memory::heap_mutex = Mutex();
GRUBMODULE Memory::modules[256];

PPML4E Memory::get_page(void* base_addr) {
	uintptr_t i = (uintptr_t) base_addr >> 12;
	PDE pde = (PDE)((uintptr_t)pagetable[(i >> 27) & 0x1FF] & 0xFFFFFFFFFFFFF000);
	if(pde == 0) return 0;
	PDPE pdpe = (PDPE)((uintptr_t)pde[(i >> 18) & 0x1FF] & 0xFFFFFFFFFFFFF000);
	if(pdpe == 0) return 0;
	PPML4E page = (PPML4E)((uintptr_t)pdpe[(i >> 9) & 0x1FF] & 0xFFFFFFFFFFFFF000);
	if(page == 0) return 0;
	return &page[i & 0x1FF];
}

static inline uintptr_t ALIGN(uintptr_t base, size_t align) {
	if (base % align == 0) return base;
	return base + align - (base % align);
}

static const void *__stack_start__ = (void*)0x1000;
static const void *__stack_end__ = (void*)0x2000;
static const _uint64 KBTS4 = 0xFFFFFFFFFFFFF000;

extern "C" {
	void *__text_start__, *__text_end__;
	void *__data_start__, *__data_end__;
	void *__modules_start__, *__modules_end__;
	void *__bss_start__, *__bss_end__;
}

void Memory::init() {
	// Filling kernel addresses
	kernel_data.kernel = (uintptr_t)&__text_start__;
	kernel_data.stack = (uintptr_t)__stack_start__;
	kernel_data.stack_top = (uintptr_t)__stack_end__;
	kernel_data.data = (uintptr_t)&__data_start__;
	kernel_data.data_top = (uintptr_t)&__data_end__;
	kernel_data.modules = (uintptr_t)&__modules_start__;
	kernel_data.modules_top = (uintptr_t)&__modules_end__;
	kernel_data.bss = (uintptr_t)&__bss_start__;
	kernel_data.bss_top = (uintptr_t)&__bss_end__;
	
	// Buffering BIOS interrupts
	for(int i = 0; i < 256; i++) {
		Interrupts::interrupts32[i] = ((PINTERRUPT32)0)[i];
	}
	
	// Buffering grub data
	kernel_data.flags = grub_data->flags;
	kernel_data.mem_lower = grub_data->mem_lower;
	kernel_data.mem_upper = grub_data->mem_upper;
	kernel_data.boot_device = grub_data->boot_device;
	kernel_data.boot_device = grub_data->boot_device;
	kernel_data.mods = (PMODULE)((uintptr_t)grub_data->pmods_addr & 0xFFFFFFFF);
	if((uintptr_t)kernel_data.mods < 0x100000)
		kernel_data.mods = (PMODULE)((uintptr_t)kernel_data.mods + 0x100000);
	kernel_data.mmap_length = grub_data->mmap_length;
	kernel_data.mmap_addr = (void*)((uintptr_t)grub_data->pmmap_addr & 0xFFFFFFFF);
	if((uintptr_t)kernel_data.mmap_addr < 0x100000)
		kernel_data.mmap_addr = (void*)((uintptr_t)kernel_data.mmap_addr + 0x100000);
	
	char cmdline[256]; int32_t cmdlinel = 0;
	if (((kernel_data.flags & 4) == 4) && (grub_data->pcmdline != 0)) {
		char* c = (char*)((uintptr_t)grub_data->pcmdline & 0xFFFFFFFF);
		if((_uint64)c < 0x100000)
			c = (char*)((uintptr_t)c + 0x100000);
		int i = 0;
		while((c[i] != 0) && (i < 255)) { cmdline[i] = c[i]; i++; }
		cmdline[i] = 0;
		cmdlinel = i+1;
	}
	uintptr_t addr;
	
#define FILL_PAGES(LOW, TOP) do {\
uintptr_t low = (LOW) & KBTS4, top = ALIGN((TOP), 0x1000);\
for(addr = low; addr < top; addr += 0x1000) \
	*(uintptr_t*)(get_page((void*)addr)) &= ~4;\
} while (0)
	
	if (((kernel_data.flags & 8) == 8) &&
		(grub_data->mods_count != 0) &&
		(kernel_data.mods != 0)) {
		modules[grub_data->mods_count].start = 0;
		PGRUBMODULE c = (PGRUBMODULE)kernel_data.mods;
		for (unsigned int i = 0; i < grub_data->mods_count; i++) {
			modules[i].start = c->start;
			modules[i].end = c->end;
			
			// Set module pages as system
			FILL_PAGES(modules[i].start & 0xFFFFF000, modules[i].end & 0xFFFFF000);
			c = (PGRUBMODULE)((uintptr_t)c + 16);
		}
	} else { kernel_data.mods = 0; }
	
	// Initialization of pagetables
	
	// BIOS Data
	*(uintptr_t*)(get_page((void*)0x00000000)) &= 0xFFFFFFFFFFFFFFF0;
	
	FILL_PAGES(0x0A0000, 0x0C8000);  // Video data & VGA BIOS
	FILL_PAGES(0x0C8000, 0x0F0000);  // Reserved for many systems
	FILL_PAGES(0x0F0000, 0x100000);  // BIOS Code
	
	FILL_PAGES(kernel_data.stack, kernel_data.stack_top);  // PXOS Stack
	FILL_PAGES(kernel_data.kernel, kernel_data.data_top);  // PXOS Code & Data
	FILL_PAGES(kernel_data.modules, kernel_data.modules_top);  // PXOS Modules
	FILL_PAGES(kernel_data.bss, kernel_data.bss_top);  // PXOS BSS

	// Page table
	*(uintptr_t*)(get_page((void*)pagetable)) &= ~4;
#undef FILL_PAGES
	
	for(uint16_t i = 0; i < 512; i++) {
		PPDE pde = pagetable[i];
		if(((uintptr_t)pde & 1) == 0) continue;
		pde = (PPDE)((uintptr_t)pde & KBTS4);
		*(uintptr_t*)(get_page((void*)pde)) &= ~4;
		for(uint32_t j = 0; j < 512; j++) {
			PPDPE pdpe = pde[j];
			if(((uintptr_t)pdpe & 1) == 0) continue;
			pdpe = (PPDPE)((uintptr_t)pdpe & KBTS4);
			*(uintptr_t*)(get_page((void*)pdpe)) &= ~4;
			for(uint16_t k = 0; k < 512; k++) {
				PPML4E pml4e = pdpe[k];
				if(((uintptr_t)pml4e & 1) == 0) continue;
				pml4e = (PPML4E)((uintptr_t)pml4e & KBTS4);
				*(uintptr_t*)(get_page((void*)pml4e)) &= ~4;
			}
		}
	}
	
	// Clearing unused pages
	for(uint16_t i = 0; i < 512; i++) {
		PPDE pde = pagetable[i];
		if(((uintptr_t)pde & 1) == 0) continue;
		pde = (PPDE)((uintptr_t)pde & 0xFFFFFFFFFFFFF000);
		for(uint16_t j = 0; j < 512; j++) {
			PPDPE pdpe = pde[j];
			if(((uintptr_t)pdpe & 1) == 0) continue;
			pdpe = (PPDPE)((uintptr_t)pdpe & 0xFFFFFFFFFFFFF000);
			for(uint16_t k = 0; k < 512; k++) {
				PPML4E pml4e = pdpe[k];
				if(((uintptr_t)pml4e & 1) == 0) continue;
				pml4e = (PPML4E)((uintptr_t)pml4e & 0xFFFFFFFFFFFFF000);
				for(uint16_t l = 0; l < 512; l++) {
					void* addr = pml4e[l];
					if(((uintptr_t)addr & 4) != 0)
						pml4e[l] = 0;
				}
			}
		}
	}
	if(cmdlinel > 0) {
		kernel_data.cmdline = (char*)alloc(cmdlinel);
		for(int i = 0; i < cmdlinel; i++) {
			kernel_data.cmdline[i] = cmdline[i];
		}
	} else {
		kernel_data.cmdline = 0;
	}
	if(((kernel_data.flags & 8) == 8) && (kernel_data.mods != 0)) {
		PMODULE mod = kernel_data.mods = (PMODULE)alloc(sizeof(MODULE));
		mod->start = 0;
		mod->end = 0;
		mod->next = 0;
		int i = 0;
		while (modules[i].start != 0) {
			mod->start = (void*)((uintptr_t)modules[i].start & 0xFFFFFFFF);
			mod->end = (void*)((uintptr_t)modules[i].end & 0xFFFFFFFF);
			i++;
			if(modules[i].start != 0)
				mod = (PMODULE)(mod->next = (void*)alloc(sizeof(MODULE)));
		}
		mod->next = (void*)0;
	}
}
void Memory::map() {
	asm("pushfq; cli");
	page_mutex.lock();
	clrscr();
	_uint64 i;
	char c = 0, nc = 0;
	_uint64 start = 0;
	for(i = 0; i < 0xFFFFF000; i+=0x1000) {
		void *p = get_page((void*)i);
		if(p != 0) nc = (*(uintptr_t*)(p)) & 0xF;
		if(nc & 1) {
			if(nc & 4) {
				nc = 'U';
			} else {
				nc = 'S';
			}
		} else {
			nc = 'E';
		}
		if (nc != c) {
			if (c != 0)
				printf("%c: %016x - %016x\n", c, start, i);
			c = nc;
			start = i;
		}
	}
	printf("%c: %016x - %016x\n", c, start, i);
	page_mutex.release();
	asm("popfq");
}
void* Memory::salloc(void* mem) {
	asm("pushfq; cli");
	page_mutex.lock();
	uintptr_t i = (uintptr_t)(mem) >> 12;
	void *addr = (void*)(i << 12);
	PDE pde = (PDE)((uintptr_t)pagetable[(i >> 27) & 0x1FF] & 0xFFFFFFFFFFFFF000);
	if(pde == 0) {
		pde = (PDE)((uintptr_t)_palloc(1));
		pagetable[(i >> 27) & 0x1FF] = (PPDE)((uintptr_t)(pde) | 3);
	}
	PDPE pdpe = (PDPE)((uintptr_t)pde[(i >> 18) & 0x1FF] & 0xFFFFFFFFFFFFF000);
	if(pdpe == 0) {
		pdpe = (PDPE)((uintptr_t)_palloc(1));
		pde[(i >> 18) & 0x1FF] = (PPML4E)((uintptr_t)(pdpe) | 3);
	}
	PPML4E page = (PPML4E)((uintptr_t)pdpe[(i >> 9) & 0x1FF] & 0xFFFFFFFFFFFFF000);
	if(page == 0) {
		page = (PPML4E)((uintptr_t)_palloc(1));
		pdpe[(i >> 9) & 0x1FF] = (void*)((uintptr_t)(page) | 3);
	}
	page[i & 0x1FF] = (void*)(((uintptr_t)addr) | 3);
	page_mutex.release();
	asm("popfq");
	return addr;
}
void* Memory::_palloc(bool sys) {
start:
	void *addr = 0; PPML4E page;
	uintptr_t i = last_page-1;
	while(i < KBTS4) {
		i++;
		addr = (void*)(i << 12);
		page = get_page(addr);
		if((page == 0) || (*(uintptr_t*)page & 1) == 0) break;
	}
	last_page = i;
	PDE pde = (PDE)((uintptr_t)pagetable[(i >> 27) & 0x1FF] & KBTS4);
	PDPE pdpe = (PDPE)((uintptr_t)pde[(i >> 18) & 0x1FF] & KBTS4);
	PDPE pdpen = (PDPE)((uintptr_t)pde[((i+1) >> 18) & 0x1FF] & KBTS4);
	page = (PPML4E)((uintptr_t)pdpe[(i >> 9) & 0x1FF] & KBTS4);
	if(((uintptr_t)pde[((i+2) >> 18) & 0x1FF] & 1) == 0) {
		page[i & 0x1FF] = (void*)((uintptr_t)addr | 3);
		i++; i++;
		pde[(i >> 18) & 0x1FF] = (void**)((uintptr_t)addr | 3);
		for(uint16_t j = 0; j < 0x200; j++)
			((uintptr_t*)addr)[j] = 0;
		goto start;
	}
	if(((_uint64)pdpen[((i+1) >> 9) & 0x1FF] & 1) == 0) {
		page[i & 0x1FF] = (void*)((uintptr_t)addr | 3);
		i++;
		pdpen[(i >> 9) & 0x1FF] = (void*)((uintptr_t)addr | 3);
		for(uint16_t j = 0; j < 0x200; j++)
			((uintptr_t*)addr)[j] = ((uintptr_t)addr + (j+1)*0x1000);
		goto start;
	}
	page[i & 0x1FF] = (void*)(((uintptr_t)addr) | (sys == 0 ? 7 : 3));
	for(i = 0; i < 0x200; i++)
		((_uint64*)addr)[i] = 0;
	return addr;
}
void* Memory::palloc(bool sys) {
	asm("pushfq; cli");
	page_mutex.lock();
	void* ret = _palloc(sys);
	page_mutex.release();
	asm("popfq");
	return ret;
}
void Memory::pfree(void* page) {
	asm("pushfq; cli");
	page_mutex.lock();
	PPML4E pdata = get_page(page);
	if((pdata != 0) && ((*(uintptr_t*)pdata & 5) == 1)) {
		*(uintptr_t*)pdata &= 0xFFFFFFFFFFFFFFF0;
		if(((uintptr_t)page >> 12) < last_page) last_page = (uintptr_t)page >> 12;
	}
	page_mutex.release();
	asm("popfq");
}
void* Memory::alloc(size_t size, size_t align) {
	if(size == 0) return (void*)0;
	heap_mutex.lock();
	uintptr_t ns = (uintptr_t)first_free, ne; char f;
	PALLOCTABLE t;
	while(1) {
		if(ns%align != 0) ns = ns + align - (ns%align);
		ne = ns + size;
		f = 0;
		uintptr_t ps = ns >> 12, pe = (ne >> 12) + (((ne & 0xFFF) !=0) ? 1 : 0);
		for(uintptr_t i = ps; i < pe; i++) {
			PPML4E pdata = get_page((void*)((uintptr_t)i << 12));
			if((pdata != 0) &&
			   ((*(uintptr_t*)pdata & 1) == 1) &&
			   ((*(uintptr_t*)pdata & 4) == 0)) {
				ns = (i+1) << 12;
				if(ns%align != 0) ns = ns + align - (ns%align);
				ne = ns + size;
				f = 1;
			}
		}
		if(f != 0) continue;
		t = allocs;
		if(t == 0) break;
		while(1) {
			for(int i = 0; i < 255; i++) {
				if(t->allocs[i].addr == 0) continue;
				uintptr_t as = (uintptr_t)t->allocs[i].addr;
				uintptr_t ae = as+(uintptr_t)t->allocs[i].size;
				if(ne < as) continue;
				if(ae < ns) continue;
				if(
				   ((ns >= as) && (ns < ae)) ||  // NA starts in alloc
				   ((ne >= as) && (ne < ae)) ||  // NA ends in alloc
				   ((ns >= as) && (ne < ae)) ||  // NA inside alloc
				   ((ns <= as) && (ne > ae))     // alloc inside NA
				   ) {
					ns = ae;
					if(ns%align != 0) ns = ns + align - (ns%align);
					ne = ns + size;
					f = 1;
				}
			}
			if(f != 0) break;
			if(t->next == 0) break;
			t = (PALLOCTABLE)t->next;
		}
		for(uintptr_t i = ps; i < pe; i++) {
			PML4E page = get_page((void*)(i*0x1000));
			if((page == 0) || (*(uintptr_t*)page & 1) == 0) {
				void *t = palloc(0);
				if((uintptr_t)t != (i * 0x1000)) {
					f = 1;
					break;
				}
			}
		}
		if(f == 0) break;
	}
	// Finding memory slot for alloc record
	if(allocs == 0)
		allocs = (PALLOCTABLE)palloc(1);
	t = allocs;
	int ai;
	while(1) {
		ai = -1;
		for(int i = 0; i < 255; i++)
			if(t->allocs[i].addr == 0) {ai = i; break;}
		if(ai == -1) {
			if(t->next == 0) {
				t->next = (PALLOCTABLE)palloc(1);
				((PALLOCTABLE)t->next)->next = 0;
			}
			t = (PALLOCTABLE)t->next;
		} else { break; }
	}
	t->allocs[ai].addr = (void*) ns;
	t->allocs[ai].size = size;
	if(((uintptr_t)first_free < ns) && (align == 4)) {
		first_free = (void*) (ns + size);
	}
	heap_mutex.release();
	return (void*) ns;
}
void Memory::free(void* addr) {
	heap_mutex.lock();
	PALLOCTABLE t = allocs;
	while(1) {
		for(int i = 0; i < 255; i++)
			if(t->allocs[i].addr == addr) {
				t->allocs[i].addr = 0;
				t->allocs[i].size = 0;
				if((uintptr_t)addr < (uintptr_t)first_free)
					first_free = addr;
				goto end;
			}
		if(t->next == 0) goto end;
		t = (PALLOCTABLE)t->next;
	}
end:
	heap_mutex.release();
}
void Memory::copy(void *dest, void *src, size_t count) {
	char *cdest = (char*)dest, *csrc = (char*)src;
	while(count) {
		*cdest = *csrc;
		cdest++;
		csrc++;
		count--;
	}
}

void* operator new(size_t a) {
	return Memory::alloc(a);
}
void operator delete(void* a) {
	return Memory::free(a);
}
