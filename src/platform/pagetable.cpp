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

#include "pagetable.hpp"
#include "efi.hpp"
#include "multiboot_info.hpp"
#include "interrupts.hpp"

PTE *Pagetable::pagetable;
Mutex Pagetable::page_mutex;
uintptr_t Pagetable::last_page = 1;

static inline void fillPages(uintptr_t low, uintptr_t top, PTE *pagetable) {
  low &= 0xFFFFFFFFFFFFF000;
  top = ALIGN(top, 0x1000);
  for (; low < top; low += 0x1000) \
    PTE::find(low, pagetable)->user = 0; \
}

static inline void fillPages(void *low, void *top, PTE *pagetable) {
  fillPages((uintptr_t)low, (uintptr_t)top, pagetable);
}

void Pagetable::init() {
  char *stack_start, *stack_top;
  char *text_start, *data_top;
  char *modules_start, *modules_top;
  char *bss_start, *bss_top;

  const EFI_SYSTEM_TABLE *ST = EFI::getSystemTable();
  MULTIBOOT_PAYLOAD *multiboot = Multiboot::getPayload();

  asm volatile("mov %%cr3, %q0":"=r"(pagetable));
  asm volatile("lea __stack_start__(%%rip), %q0":"=r"(stack_start));
  asm volatile("lea __stack_end__(%%rip), %q0":"=r"(stack_top));
  asm volatile("lea __text_start__(%%rip), %q0":"=r"(text_start));
  asm volatile("lea __data_end__(%%rip), %q0":"=r"(data_top));
  asm volatile("lea __modules_start__(%%rip), %q0":"=r"(modules_start));
  asm volatile("lea __modules_end__(%%rip), %q0":"=r"(modules_top));
  asm volatile("lea __bss_start__(%%rip), %q0":"=r"(bss_start));
  asm volatile("lea __bss_end__(%%rip), %q0":"=r"(bss_top));

  // Initialization of pagetables

  if (ST) {
    void *p = 0;
    ST->BootServices->AllocatePages(
        EFI_ALLOCATE_TYPE_ADDR, EFI_MEMORY_TYPE_UNUSABLE, 1, &p);
  } else {
    // BIOS Data
    PTE::find((uintptr_t)0, pagetable)->present = 0;

    fillPages(0x09F000, 0x0A0000, pagetable);  // Extended BIOS Data
    fillPages(0x0A0000, 0x0C8000, pagetable);  // Video data & VGA BIOS
    fillPages(0x0C8000, 0x0F0000, pagetable);  // Reserved for many systems
    fillPages(0x0F0000, 0x100000, pagetable);  // BIOS Code

    fillPages(stack_start, stack_top, pagetable);  // PXOS Stack
    fillPages(text_start, data_top, pagetable);  // PXOS Code & Data
    fillPages(modules_start, modules_top, pagetable);  // PXOS Modules
    fillPages(bss_start, bss_top, pagetable);  // PXOS BSS
    if (multiboot)
      fillPages(multiboot, multiboot+1, pagetable);

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
  }

  if (multiboot) {
    if (multiboot->flags & MB_FLAG_CMDLINE) {
      if (multiboot->pcmdline < 0x80000)
        multiboot->pcmdline += (uintptr_t)bss_top;
      map(reinterpret_cast<void*>(multiboot->pcmdline));
    }

    if (multiboot->flags & MB_FLAG_MODS) {
      if (multiboot->pmods_addr < 0x80000)
        multiboot->pmods_addr += (uintptr_t)bss_top;

      uintptr_t low = (uintptr_t)multiboot->pmods_addr & 0xFFFFFFFFFFFFF000;
      uintptr_t top = ALIGN(
          (uintptr_t)multiboot->pmods_addr +
          multiboot->mods_count * sizeof(MULTIBOOT_MODULE),
          0x1000);
      for (uintptr_t addr = low; addr < top; addr += 0x1000)
        map(reinterpret_cast<void*>(addr));

      const MULTIBOOT_MODULE *mods =
          reinterpret_cast<MULTIBOOT_MODULE*>(multiboot->pmods_addr);
      for (uint32_t i = 0; i < multiboot->mods_count; i++) {
        uintptr_t low = mods[i].start;
        uintptr_t top = ALIGN(mods[i].end, 0x1000);
        for (uintptr_t addr = low; addr < top; addr += 0x1000)
          map(reinterpret_cast<void*>(addr));
      }
    }

    if (multiboot->flags & MB_FLAG_MEMMAP) {
      if (multiboot->pmmap_addr < 0x80000)
        multiboot->pmmap_addr += (uintptr_t)bss_top;

      const char *mmap = reinterpret_cast<const char*>(multiboot->pmmap_addr);
      const char *mmap_top = mmap + multiboot->mmap_length;
      while (mmap < mmap_top) {
        const MULTIBOOT_MMAP_ENT *ent =
            reinterpret_cast<const MULTIBOOT_MMAP_ENT*>(mmap);
        map(ent);
        if (ent->type != 1) {
          uintptr_t low = (uintptr_t)ent->base & 0xFFFFFFFFFFFFF000;
          uintptr_t top = ALIGN((uintptr_t)ent->base + ent->length, 0x1000);
          for (uintptr_t addr = low; addr < top; addr += 0x1000)
            map(reinterpret_cast<void*>(addr));
        }
        mmap += ent->size + sizeof(ent->size);
      }
    }
  }
}

void* Pagetable::map(const void* mem) {
  uint64_t t = EnterCritical();
  page_mutex.lock();

  const EFI_SYSTEM_TABLE *ST = EFI::getSystemTable();
  if (ST) {
    void *ptr = reinterpret_cast<void*>((uintptr_t)mem);
    ST->BootServices->AllocatePages(
        EFI_ALLOCATE_TYPE_ADDR, EFI_MEMORY_TYPE_DATA, 1, &ptr);
    page_mutex.release();
    LeaveCritical(t);
    return ptr;
  }

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
  const EFI_SYSTEM_TABLE *ST = EFI::getSystemTable();
  if (ST) {
    size_t mapsize = 0;
    EFI_MEMORY_DESCRIPTOR *map = 0, *ent, *top;
    uint64_t key = 0;
    size_t entsize = 0;
    uint32_t ver;
    ST->BootServices->GetMemoryMap(&mapsize, map, &key, &entsize, &ver);
    map = reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(alloca(mapsize));
    ST->BootServices->GetMemoryMap(&mapsize, map, &key, &entsize, &ver);

    ent = map;
    uintptr_t min = nolow ? 0x100000 : 0x1000;
    void *ptr = 0;
    top = reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>((uintptr_t)map + mapsize);
    for (ent = map;
        ent < top;
        ent = reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(
            (uintptr_t)ent + entsize)) {
      if (ent->Type != EFI_MEMORY_TYPE_CONVENTIONAL) continue;
      if ((ent->PhysicalStart + ent->NumberOfPages * 0x1000) <= min)
        continue;
      ptr = reinterpret_cast<void*>(MAX(ent->PhysicalStart, min));
      break;
    }

    ST->BootServices->AllocatePages(
        EFI_ALLOCATE_TYPE_ADDR, EFI_MEMORY_TYPE_DATA, 1, &ptr);
    Memory::fill(ptr, 0, 0x1000);
    return ptr;
  }

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
  const EFI_SYSTEM_TABLE *ST = EFI::getSystemTable();
  if (ST) {
    ST->BootServices->FreePages(page, 1);
  } else {
    PTE *pdata = PTE::find(page, pagetable);
    if ((pdata != 0) && pdata->present) {
      pdata->present = 0;
      void *addr = pdata->getPtr();
      if (((uintptr_t)addr >> 12) < last_page)
        last_page = (uintptr_t)addr >> 12;
    }
  }
  page_mutex.release();
  LeaveCritical(t);
}
