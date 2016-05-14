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

static const _uint64 KBTS4 = 0xFFFFFFFFFFFFF000;

extern "C" {
	extern void *__first_page__;
	extern void *__pagetable__;
	extern void *__stack_start__, *__stack_end__;
	extern void *__text_start__, *__text_end__;
	extern void *__data_start__, *__data_end__;
	extern void *__modules_start__, *__modules_end__;
	extern void *__bss_start__, *__bss_end__;
}

PPTE Memory::pagetable = (PPTE)&__pagetable__;
PALLOCTABLE Memory::allocs = 0;
void* Memory::first_free = (void*)&__first_page__;
uintptr_t Memory::last_page = 1;
Mutex Memory::page_mutex = Mutex();
Mutex Memory::heap_mutex = Mutex();
GRUBMODULE Memory::modules[256];

PPTE Memory::get_page(void* base_addr) {
	uintptr_t i = (uintptr_t) base_addr >> 12;
	PTE addr;
	addr = pagetable[(i >> 27) & 0x1FF];
	PPTE pde = addr.present ? (PPTE)PTE_GET_PTR(addr) : 0;
	if(pde == 0) return 0;
	addr = pde[(i >> 18) & 0x1FF];
	PPTE pdpe = addr.present ? (PPTE)PTE_GET_PTR(addr) : 0;
	if(pdpe == 0) return 0;
	addr = pdpe[(i >> 9) & 0x1FF];
	PPTE page = addr.present ? (PPTE)PTE_GET_PTR(addr) : 0;
	if(page == 0) return 0;
	return &page[i & 0x1FF];
}

static inline uintptr_t ALIGN(uintptr_t base, size_t align) {
	if (base % align == 0) return base;
	return base + align - (base % align);
}

void Memory::init() {
	// Filling kernel addresses
	kernel_data.kernel = (uintptr_t)&__text_start__;
	kernel_data.stack = (uintptr_t)&__stack_start__;
	kernel_data.stack_top = (uintptr_t)&__stack_end__;
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
	if((uintptr_t)kernel_data.mods < (uintptr_t)&__bss_end__)
		kernel_data.mods = (PMODULE)((uintptr_t)kernel_data.mods +
									 (uintptr_t)&__bss_end__);
	kernel_data.mmap_length = grub_data->mmap_length;
	kernel_data.mmap_addr = (void*)((uintptr_t)grub_data->pmmap_addr & 0xFFFFFFFF);
	if((uintptr_t)kernel_data.mmap_addr < (uintptr_t)&__bss_end__)
		kernel_data.mmap_addr = (void*)((uintptr_t)kernel_data.mmap_addr +
										(uintptr_t)&__bss_end__);
	
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
	
#define FILL_PAGES(LOW, TOP) do {\
uintptr_t low = (LOW) & KBTS4, top = ALIGN((TOP), 0x1000);\
for(uintptr_t addr = low; addr < top; addr += 0x1000) \
	get_page((void*)addr)->user = 0;\
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
	get_page(0)->present = 0;
	
	FILL_PAGES(0x0A0000, 0x0C8000);  // Video data & VGA BIOS
	FILL_PAGES(0x0C8000, 0x0F0000);  // Reserved for many systems
	FILL_PAGES(0x0F0000, 0x100000);  // BIOS Code
	
	FILL_PAGES(kernel_data.stack, kernel_data.stack_top);  // PXOS Stack
	FILL_PAGES(kernel_data.kernel, kernel_data.data_top);  // PXOS Code & Data
	FILL_PAGES(kernel_data.modules, kernel_data.modules_top);  // PXOS Modules
	FILL_PAGES(kernel_data.bss, kernel_data.bss_top);  // PXOS BSS

	// Page table
	get_page(pagetable)->user = 0;
#undef FILL_PAGES
	
	PTE pte;
	for(uint16_t i = 0; i < 512; i++) {
		pte = pagetable[i];
		if(!pte.present) continue;
		PPTE pde = (PPTE)PTE_GET_PTR(pte);
		get_page(pde)->user = 0;
		for(uint32_t j = 0; j < 512; j++) {
			pte = pde[j];
			if(!pte.present) continue;
			PPTE pdpe = (PPTE)PTE_GET_PTR(pte);
			get_page(pdpe)->user = 0;
			for(uint16_t k = 0; k < 512; k++) {
				pte = pdpe[k];
				if(!pte.present) continue;
				PPTE pml4e = (PPTE)PTE_GET_PTR(pte);
				get_page(pml4e)->user = 0;
			}
		}
	}
	
	// Clearing unused pages
	for(uint16_t i = 0; i < 512; i++) {
		pte = pagetable[i];
		if(!pte.present) continue;
		PPTE pde = (PPTE)PTE_GET_PTR(pte);
		get_page(pde)->user = 0;
		for(uint32_t j = 0; j < 512; j++) {
			pte = pde[j];
			if(!pte.present) continue;
			PPTE pdpe = (PPTE)PTE_GET_PTR(pte);
			get_page(pdpe)->user = 0;
			for(uint16_t k = 0; k < 512; k++) {
				pte = pdpe[k];
				if(!pte.present) continue;
				PPTE pml4e = (PPTE)PTE_GET_PTR(pte);
				for(uint16_t l = 0; l < 512; l++) {
					if (pml4e[l].user)
						pml4e[l].present = 0;
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
		PPTE page = get_page((void*)i);
		nc = !page ? 0 : page->flags;
		if((nc & 1) != 0) {
			if((nc & 4) != 0) {
				nc = 'U';
			} else {
				nc = 'S';
			}
		} else {
			nc = 'E';
		}
		if (nc != c) {
			if (c != 0 && c != 'E')
				printf("%c: %016x - %016x\n", c, start, i);
			c = nc;
			start = i;
		}
	}
	if (c != 0 && c != 'E')
		printf("%c: %016x - %016x\n", c, start, i);
	page_mutex.release();
	asm("popfq");
}
void* Memory::salloc(void* mem) {
	asm("pushfq; cli");
	page_mutex.lock();
	uintptr_t i = (uintptr_t)(mem) >> 12;
	void *addr = (void*)(i << 12);
	PTE pte = pagetable[(i >> 27) & 0x1FF];
	if(!pte.present) {
		pagetable[(i >> 27) & 0x1FF] = pte = PTE_MAKE(_palloc(0, true), 3);
	}
	PPTE pde = (PPTE)PTE_GET_PTR(pte);
	pte = pde[(i >> 18) & 0x1FF];
	if(!pte.present) {
		pde[(i >> 18) & 0x1FF] = pte = PTE_MAKE(_palloc(0, true), 3);
	}
	PPTE pdpe = (PPTE)PTE_GET_PTR(pte);
	pte = pdpe[(i >> 9) & 0x1FF];
	if(!pte.present) {
		pdpe[(i >> 9) & 0x1FF] = pte = PTE_MAKE(_palloc(0, true), 3);
	}
	PPTE page = (PPTE)PTE_GET_PTR(pte);
	page[i & 0x1FF] = PTE_MAKE(addr, 3);
	page_mutex.release();
	asm("popfq");
	return addr;
}
void* Memory::_palloc(uint8_t avl, bool nolow) {
start:
	void *addr = 0; PPTE page;
	uintptr_t i = last_page-1;
	if (nolow && (i < 0x100))
		i = 0x100;
	while(i < KBTS4) {
		i++;
		addr = (void*)(i << 12);
		page = get_page(addr);
		if((page == 0) || (*(uintptr_t*)page & 1) == 0) break;
	}
	if (!nolow) last_page = i;
	PPTE pde = (PPTE)PTE_GET_PTR(pagetable[(i >> 27) & 0x1FF]);
	PPTE pdpe = (PPTE)PTE_GET_PTR(pde[(i >> 18) & 0x1FF]);
	PPTE pdpen = (PPTE)PTE_GET_PTR(pde[((i+1) >> 18) & 0x1FF]);
	page = (PPTE)PTE_GET_PTR(pdpe[(i >> 9) & 0x1FF]);
	if(!pde[((i+2) >> 18) & 0x1FF].present) {
		page[i & 0x1FF] = PTE_MAKE(addr, 3);
		i++; i++;
		pde[(i >> 18) & 0x1FF] = PTE_MAKE(addr, 3);
		for(uint16_t j = 0; j < 0x200; j++)
			((uintptr_t*)addr)[j] = 0;
		goto start;
	}
	if(!pdpen[((i+1) >> 9) & 0x1FF].present) {
		page[i & 0x1FF] = PTE_MAKE(addr, 3);
		i++;
		pdpen[(i >> 9) & 0x1FF] = PTE_MAKE(addr, 3);
		for(uint16_t j = 0; j < 0x200; j++)
			((uintptr_t*)addr)[j] = 0;
		goto start;
	}
	page[i & 0x1FF] = PTE_MAKE_AVL(addr, avl, 3);
	for(i = 0; i < 0x200; i++)
		((_uint64*)addr)[i] = 0;
	return addr;
}
void* Memory::palloc(uint8_t avl) {
	asm("pushfq; cli");
	page_mutex.lock();
	void* ret = _palloc(avl);
	page_mutex.release();
	asm("popfq");
	return ret;
}
void Memory::pfree(void* page) {
	asm("pushfq; cli");
	page_mutex.lock();
	PPTE pdata = get_page(page);
	if((pdata != 0) && pdata->present) {
		pdata->present = 0;
		void *addr = PTE_GET_PTR(*pdata);
		if(((uintptr_t)addr >> 12) < last_page)
			last_page = (uintptr_t)addr >> 12;
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
			PPTE pdata = get_page((void*)((uintptr_t)i << 12));
			if((pdata != 0) &&
			   (pdata->present) &&
			   (pdata->avl == 0)) {
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
			PPTE page = get_page((void*)(i*0x1000));
			if((page == 0) || !page->present) {
				void *t = palloc(1);
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
		allocs = (PALLOCTABLE)palloc();
	t = allocs;
	int ai;
	while(1) {
		ai = -1;
		for(int i = 0; i < 255; i++)
			if(t->allocs[i].addr == 0) {ai = i; break;}
		if(ai == -1) {
			if(t->next == 0) {
				t->next = (PALLOCTABLE)palloc();
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
	if (addr == 0) return;
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
void *Memory::realloc(void *addr, size_t size, size_t align) {
	if (size == 0) return 0;
	if (addr == 0) return alloc(size, align);
	heap_mutex.lock();
	PALLOCTABLE t = allocs;
	size_t oldsize = 0;
	while (t != 0) {
		for(int i = 0; i < 255; i++) {
			if (t->allocs[i].addr == addr) {
				oldsize = t->allocs[i].size;
				break;
			}
		}
		if (oldsize != 0) break;
		t = (PALLOCTABLE)t->next;
	}
	heap_mutex.release();
	if (oldsize == 0) return 0;
	void *newptr = Memory::alloc(size, align);
	copy(newptr, addr, oldsize);
	free(addr);
	return newptr;
}
void Memory::copy(void *dest, void *src, size_t count) {
	asm(
		"mov %0, %%rsi;"
		"mov %1, %%rdi;"
		"cld;"
		"rep movsb;"
		::"r"(src),"r"(dest),"c"(count)
		);
}

void* operator new(size_t a) {
	return Memory::alloc(a);
}
void operator delete(void* a) {
	return Memory::free(a);
}
