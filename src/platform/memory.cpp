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
#include "interrupts.hpp"

extern "C" {
  extern char __first_page__;
  extern PTE __pagetable__;
}

PTE *Pagetable::pagetable = &__pagetable__;
Mutex Pagetable::page_mutex = Mutex();
uintptr_t Pagetable::last_page = 1;

ALLOCTABLE *Heap::allocs = 0;
void* Heap::first_free = &__first_page__;
Mutex Heap::heap_mutex = Mutex();

static inline void fillPages(uintptr_t low, uintptr_t top,
                             PTE *pagetable = &__pagetable__) {
  low &= 0xFFFFFFFFFFFFF000;
  top = ALIGN(top, 0x1000);
  for (; low < top; low += 0x1000) \
    PTE::find(low, pagetable)->user = 0; \
}

static inline void fillPages(void *low, void *top,
                             PTE *pagetable = &__pagetable__) {
  fillPages((uintptr_t)low, (uintptr_t)top, pagetable);
}

void Pagetable::init() {
  extern char __stack_start__, __stack_end__;
  extern char __text_start__, __data_end__;
  extern char __modules_start__, __modules_end__;
  extern char __bss_start__, __bss_end__;

  // Buffering BIOS interrupts
  Memory::copy(Interrupts::interrupts32, 0, sizeof(INTERRUPT32) * 256);

  // Buffering grub data
  kernel_data.flags = grub_data->flags;
  kernel_data.mem_lower = grub_data->mem_lower;
  kernel_data.mem_upper = grub_data->mem_upper;
  kernel_data.boot_device = grub_data->boot_device;
  kernel_data.boot_device = grub_data->boot_device;
  kernel_data.mods = reinterpret_cast<MODULE*>(grub_data->pmods_addr);
  if ((uintptr_t)kernel_data.mods < (uintptr_t)&__bss_end__)
    kernel_data.mods = reinterpret_cast<MODULE*>((uintptr_t)kernel_data.mods
        + (uintptr_t)&__bss_end__);
  kernel_data.mmap_length = grub_data->mmap_length;
  kernel_data.mmap_addr = reinterpret_cast<char*>(grub_data->pmmap_addr);
  if ((uintptr_t)kernel_data.mmap_addr < (uintptr_t)&__bss_end__)
    kernel_data.mmap_addr += (uintptr_t)&__bss_end__;

  static GRUBMODULE modules[256];
  static char cmdline[256];
  size_t cmdlinel = 0;

  if (((kernel_data.flags & 4) == 4) && (grub_data->pcmdline != 0)) {
    const char* c = reinterpret_cast<const char*>(grub_data->pcmdline);
    if ((uintptr_t)c < 0x100000) c += 0x100000;
    int len = strlen(c, 255);
    Memory::copy(cmdline, c, len);
    cmdline[len] = 0;
    cmdlinel = len;
  }

  if (((kernel_data.flags & 8) == 8) && (grub_data->mods_count != 0)
      && (kernel_data.mods != 0)) {
    modules[grub_data->mods_count].start = 0;
    GRUBMODULE *c = reinterpret_cast<GRUBMODULE*>(kernel_data.mods);
    for (unsigned int i = 0; i < grub_data->mods_count; i++) {
      modules[i].start = c->start;
      modules[i].end = c->end;

      // Set module pages as system
      fillPages(modules[i].start & 0xFFFFF000, modules[i].end & 0xFFFFF000);
      c++;
    }
  } else {
    kernel_data.mods = 0;
  }

  // Initialization of pagetables

  // BIOS Data
  PTE::find((uintptr_t)0, pagetable)->present = 0;

  fillPages(0x09F000, 0x0A0000);  // Extended BIOS Data
  fillPages(0x0A0000, 0x0C8000);  // Video data & VGA BIOS
  fillPages(0x0C8000, 0x0F0000);  // Reserved for many systems
  fillPages(0x0F0000, 0x100000);  // BIOS Code

  fillPages(&__stack_start__, &__stack_end__);  // PXOS Stack
  fillPages(&__text_start__, &__data_end__);  // PXOS Code & Data
  fillPages(&__modules_start__, &__modules_end__);  // PXOS Modules
  fillPages(&__bss_start__, &__bss_end__);  // PXOS BSS
  fillPages(kernel_data.mmap_addr,
            kernel_data.mmap_addr + kernel_data.mmap_length);

  // Page table
  PTE::find(pagetable, pagetable)->user = 0;

  for (uint16_t i = 0; i < 512; i++) {
    if (!pagetable[i].present) continue;
    PTE *pde = pagetable[i].getPTE();
    PTE::find(pde, pagetable)->user = 0;
    for (uint32_t j = 0; j < 512; j++) {
      if (!pde[j].present) continue;
      PTE *pdpe = pde[j].getPTE();
      PTE::find(pdpe, pagetable)->user = 0;
      for (uint16_t k = 0; k < 512; k++) {
        if (!pdpe[k].present) continue;
        PTE *pml4e = pdpe[k].getPTE();
        PTE::find(pml4e, pagetable)->user = 0;
      }
    }
  }

  // Clearing unused pages
  for (uint16_t i = 0; i < 512; i++) {
    if (!pagetable[i].present) continue;
    PTE *pde = pagetable[i].getPTE();
    PTE::find(pde, pagetable)->user = 0;
    for (uint32_t j = 0; j < 512; j++) {
      if (!pde[j].present) continue;
      PTE *pdpe = pde[j].getPTE();
      PTE::find(pdpe, pagetable)->user = 0;
      for (uint16_t k = 0; k < 512; k++) {
        if (!pdpe[k].present) continue;
        PTE *pml4e = pdpe[k].getPTE();
        for (uint16_t l = 0; l < 512; l++) {
          if (pml4e[l].user)
            pml4e[l].present = 0;
        }
      }
    }
  }

  if (cmdlinel > 0) {
    kernel_data.cmdline = new char[cmdlinel+1];
    Memory::copy(kernel_data.cmdline, cmdline, cmdlinel + 1);
  } else {
    kernel_data.cmdline = 0;
  }

  if (((kernel_data.flags & 8) == 8) && (kernel_data.mods != 0)) {
    MODULE *mod = new MODULE();
    kernel_data.mods = mod;
    mod->start = 0;
    mod->end = 0;
    mod->next = 0;
    int i = 0;
    while (modules[i].start != 0) {
      mod->start = reinterpret_cast<void*>(modules[i].start);
      mod->end = reinterpret_cast<void*>(modules[i].end);
      i++;
      if (modules[i].start != 0)
        mod = (mod->next = new MODULE());
    }
    mod->next = 0;
  }

  // GRUB info
  const char *mmap = reinterpret_cast<const char*>(kernel_data.mmap_addr);
  const char *mmap_top = mmap + kernel_data.mmap_length;
  while (mmap < mmap_top) {
    const GRUBMEMENT *ent = reinterpret_cast<const GRUBMEMENT*>(mmap);
    if (ent->type != 1) {
      uintptr_t low = (uintptr_t)ent->base & 0xFFFFFFFFFFFFF000;
      uintptr_t top = ALIGN((uintptr_t)ent->base + ent->length, 0x1000);
      for (uintptr_t addr = low; addr < top; addr += 0x1000)
        map(reinterpret_cast<void*>(addr));
    }
    mmap += ent->size + sizeof(ent->size);
  }
}
void* Pagetable::map(const void* mem) {
  uint64_t t = EnterCritical();
  page_mutex.lock();
  uintptr_t i = (uintptr_t)(mem) >> 12;
  void *addr = reinterpret_cast<void*>(i << 12);
  PTE pte = pagetable[(i >> 27) & 0x1FF];
  if (!pte.present) {
    pagetable[(i >> 27) & 0x1FF] = pte = PTE(_alloc(0, true), 3);
  }
  PTE *pde = pte.getPTE();
  pte = pde[(i >> 18) & 0x1FF];
  if (!pte.present) {
    pde[(i >> 18) & 0x1FF] = pte = PTE(_alloc(0, true), 3);
  }
  PTE *pdpe = pte.getPTE();
  pte = pdpe[(i >> 9) & 0x1FF];
  if (!pte.present) {
    pdpe[(i >> 9) & 0x1FF] = pte = PTE(_alloc(0, true), 3);
  }
  PTE *page = pte.getPTE();
  page[i & 0x1FF] = PTE(addr, 3);
  page_mutex.release();
  LeaveCritical(t);
  return addr;
}
void* Pagetable::_alloc(uint8_t avl, bool nolow) {
start:
  void *addr = 0;
  PTE *page;
  uintptr_t i = last_page - 1;
  if (nolow && (i < 0x100))
    i = 0x100;
  while (i < 0xFFFFFFFFFFFFF000) {
    i++;
    addr = reinterpret_cast<void*>(i << 12);
    page = PTE::find(addr, pagetable);
    if ((page == 0) || !page->present)
      break;
  }
  if (!nolow)
    last_page = i;
  PTE *pde = pagetable[(i >> 27) & 0x1FF].getPTE();
  PTE *pdpe = pde[(i >> 18) & 0x1FF].getPTE();
  PTE *pdpen = pde[((i + 1) >> 18) & 0x1FF].getPTE();
  page = pdpe[(i >> 9) & 0x1FF].getPTE();
  if (!pde[((i + 2) >> 18) & 0x1FF].present) {
    page[i & 0x1FF] = PTE(addr, 3);
    i++;
    i++;
    pde[(i >> 18) & 0x1FF] = PTE(addr, 3);
    Memory::fill(addr, 0, 0x1000);
    goto start;
  }
  if (!pdpen[((i + 1) >> 9) & 0x1FF].present) {
    page[i & 0x1FF] = PTE(addr, 3);
    i++;
    pdpen[(i >> 9) & 0x1FF] = PTE(addr, 3);
    Memory::fill(addr, 0, 0x1000);
    goto start;
  }
  page[i & 0x1FF] = PTE(addr, avl, 3);
  Memory::fill(addr, 0, 0x1000);
  return addr;
}
void* Pagetable::alloc(uint8_t avl) {
  uint64_t t = EnterCritical();
  page_mutex.lock();
  void* ret = _alloc(avl);
  page_mutex.release();
  LeaveCritical(t);
  return ret;
}
void Pagetable::free(void* page) {
  uint64_t t = EnterCritical();
  page_mutex.lock();
  PTE *pdata = PTE::find(page, pagetable);
  if ((pdata != 0) && pdata->present) {
    pdata->present = 0;
    void *addr = pdata->getPtr();
    if (((uintptr_t)addr >> 12) < last_page)
      last_page = (uintptr_t)addr >> 12;
  }
  page_mutex.release();
  LeaveCritical(t);
}
void* Heap::alloc(size_t size, size_t align) {
  if (size == 0)
    return 0;
  heap_mutex.lock();
  uintptr_t ns = (uintptr_t)first_free, ne;
  char f;
  ALLOCTABLE *t;
  while (1) {
    if (ns % align != 0)
      ns = ns + align - (ns % align);
    ne = ns + size;
    f = 0;
    uintptr_t ps = ns >> 12, pe = (ne >> 12) + (((ne & 0xFFF) != 0) ? 1 : 0);
    for (uintptr_t i = ps; i < pe; i++) {
      // TODO: leave Heap away from pagetable
      PTE *pdata = PTE::find(i << 12, &__pagetable__);
      if ((pdata != 0) && (pdata->present) && (pdata->avl == 0)) {
        ns = (i + 1) << 12;
        if (ns % align != 0)
          ns = ns + align - (ns % align);
        ne = ns + size;
        f = 1;
      }
    }
    if (f != 0)
      continue;
    t = allocs;
    if (t == 0)
      break;
    while (1) {
      for (int i = 0; i < 255; i++) {
        if (t->allocs[i].addr == 0)
          continue;
        uintptr_t as = (uintptr_t)t->allocs[i].addr;
        uintptr_t ae = as + (uintptr_t)t->allocs[i].size;
        if (ne < as)
          continue;
        if (ae < ns)
          continue;
        if (((ns >= as) && (ns < ae)) ||  // NA starts in alloc
        ((ne >= as) && (ne < ae))
            ||  // NA ends in alloc
            ((ns >= as) && (ne < ae))
            ||  // NA inside alloc
            ((ns <= as) && (ne > ae))     // alloc inside NA
            ) {
          ns = ae;
          if (ns % align != 0)
            ns = ns + align - (ns % align);
          ne = ns + size;
          f = 1;
        }
      }
      if (f != 0)
        break;
      if (t->next == 0)
        break;
      t = t->next;
    }
    for (uintptr_t i = ps; i < pe; i++) {
      // TODO: leave Heap away from pagetable
      PTE *page = PTE::find(i << 12, &__pagetable__);
      if ((page == 0) || !page->present) {
        void *t = Pagetable::alloc(1);
        if ((uintptr_t)t != (i << 12)) {
          f = 1;
          break;
        }
      }
    }
    if (f == 0)
      break;
  }
  // Finding memory slot for alloc record
  if (allocs == 0)
    allocs = static_cast<ALLOCTABLE*>(Pagetable::alloc());
  t = allocs;
  int ai;
  while (1) {
    ai = -1;
    for (int i = 0; i < 255; i++)
      if (t->allocs[i].addr == 0) {
        ai = i;
        break;
      }
    if (ai == -1) {
      if (t->next == 0) {
        t->next = static_cast<ALLOCTABLE*>(Pagetable::alloc());
        t->next->next = 0;
      }
      t = t->next;
    } else {
      break;
    }
  }
  t->allocs[ai].addr = reinterpret_cast<void*>(ns);
  t->allocs[ai].size = size;
  if (((uintptr_t)first_free < ns) && (align == 4)) {
    first_free = reinterpret_cast<void*>(ns + size);
  }
  heap_mutex.release();
  return reinterpret_cast<void*>(ns);
}
void Heap::free(void* addr) {
  if (addr == 0)
    return;
  heap_mutex.lock();
  ALLOCTABLE *t = allocs;
  while (1) {
    for (int i = 0; i < 255; i++) {
      if (t->allocs[i].addr == addr) {
        t->allocs[i].addr = 0;
        t->allocs[i].size = 0;
        if ((uintptr_t)addr < (uintptr_t)first_free)
          first_free = addr;
        goto end;
      }
    }
    if (t->next == 0)
      goto end;
    t = t->next;
  }
  end: heap_mutex.release();
}
void *Heap::realloc(void *addr, size_t size, size_t align) {
  if (size == 0)
    return 0;
  if (addr == 0)
    return alloc(size, align);
  heap_mutex.lock();
  ALLOCTABLE *t = allocs;
  size_t oldsize = 0;
  while (t != 0) {
    for (int i = 0; i < 255; i++) {
      if (t->allocs[i].addr == addr) {
        oldsize = t->allocs[i].size;
        break;
      }
    }
    if (oldsize != 0)
      break;
    t = t->next;
  }
  heap_mutex.release();
  if (oldsize == 0)
    return 0;
  void *newptr = alloc(size, align);
  Memory::copy(newptr, addr, oldsize);
  free(addr);
  return newptr;
}

void* operator new(size_t a) {
  return Heap::alloc(a);
}
void* operator new[](size_t a) {
  return Heap::alloc(a);
}
void* operator new(size_t, size_t a) {
  return Heap::alloc(a);
}
void operator delete(void* a) {
  return Heap::free(a);
}
void operator delete[](void* a) {
  return Heap::free(a);
}
