//    PhoeniX OS Memory subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "pagetable.hpp"
#include "efi.hpp"
#include "multiboot_info.hpp"
#include "interrupts.hpp"

PTE *Pagetable::pagetable;
Mutex Pagetable::page_mutex;
uintptr_t Pagetable::last_page = 1;

static void fillPages(uintptr_t low, uintptr_t top, PTE *pagetable) {
  low &= 0xFFFFFFFFFFFFF000;
  top = ALIGN(top, 0x1000);
  for (; low < top; low += 0x1000)
    *PTE::find(low, pagetable) = PTE(low, 3);
}

static inline void fillPages(void *low, void *top, PTE *pagetable) {
  fillPages((uintptr_t)low, (uintptr_t)top, pagetable);
}

static void *efiAllocatePage(uintptr_t min, const EFI_SYSTEM_TABLE *ST) {
  size_t mapSize = 0, entSize = 0;
  EFI_MEMORY_DESCRIPTOR *map = 0, *ent;
  uint64_t mapKey;
  uint32_t entVer = 0;

  void *ptr = 0;

  ST->BootServices->GetMemoryMap(&mapSize, map, &mapKey, &entSize, &entVer);
  map = static_cast<EFI_MEMORY_DESCRIPTOR*>(alloca(mapSize));
  ST->BootServices->GetMemoryMap(&mapSize, map, &mapKey, &entSize, &entVer);
  for (ent = map;
      ent < reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(
          (uintptr_t)map + mapSize);
      ent = reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(
          (uintptr_t)ent + entSize)) {
    if (ent->Type != EFI_MEMORY_TYPE_CONVENTIONAL) continue;
    if (ent->PhysicalStart + ent->NumberOfPages * 0x1000 <= min) continue;
    ptr = reinterpret_cast<void*>(MAX(min, ent->PhysicalStart));
    break;
  }
  ST->BootServices->AllocatePages(
      ptr ? EFI_ALLOCATE_TYPE_ADDR : EFI_ALLOCATE_TYPE_ANY,
      EFI_MEMORY_TYPE_DATA, 1, &ptr);
  Memory::zero(ptr, 0x1000);
  return ptr;
}

static void efiMapPage(PTE *pagetable, const void *page,
                       const EFI_SYSTEM_TABLE *ST) {
  uintptr_t ptr = (uintptr_t)page, min = (uintptr_t)pagetable;
  uint64_t ptx = (ptr >> (12 + 9*3)) & 0x1FF;
  uint64_t pdx = (ptr >> (12 + 9*2)) & 0x1FF;
  uint64_t pdpx = (ptr >> (12 + 9)) & 0x1FF;
  uint64_t pml4x = (ptr >> 12) & 0x1FF;

  PTE *pte = pagetable + ptx;
  if (!pte->present) {
    *pte = PTE(efiAllocatePage(min, ST), 3);
    efiMapPage(pagetable, pte->getPtr(), ST);
  }
  PTE *pde = pte->getPTE() + pdx;
  if (!pde->present) {
    *pde = PTE(efiAllocatePage(min, ST), 3);
    efiMapPage(pagetable, pde->getPtr(), ST);
  }
  PTE *pdpe = pde->getPTE() + pdpx;
  if (!pdpe->present) {
    *pdpe = PTE(efiAllocatePage(min, ST), 3);
    efiMapPage(pagetable, pdpe->getPtr(), ST);
  }
  PTE *pml4e = pdpe->getPTE() + pml4x;
  *pml4e = PTE((uintptr_t)page, 3);
}

void Pagetable::init() {
  char *stack_start, *stack_top;
  char *text_start, *data_top;
  char *bss_start, *bss_top;

  const EFI_SYSTEM_TABLE *ST = EFI::getSystemTable();
  MULTIBOOT_PAYLOAD *multiboot = Multiboot::getPayload();

  static const size_t stack_size = 0x1000;

  asm volatile("mov %%cr3, %q0":"=r"(pagetable));
  asm volatile("lea __stack_end__(%%rip), %q0":"=r"(stack_top));
  stack_start = stack_top - stack_size;
  asm volatile("lea __text_start__(%%rip), %q0":"=r"(text_start));
  asm volatile("lea __data_end__(%%rip), %q0":"=r"(data_top));
  asm volatile("lea __bss_start__(%%rip), %q0":"=r"(bss_start));
  asm volatile("lea __bss_end__(%%rip), %q0":"=r"(bss_top));

  // Initialization of pagetables

  if (ST) {
    EFI_LOADED_IMAGE *loaded_image = 0;
    ST->BootServices->HandleProtocol(
        EFI::getImageHandle(), &EFI_LOADED_IMAGE_PROTOCOL,
        reinterpret_cast<void**>(&loaded_image));

    uintptr_t ptbase = 0x600000 + (RAND::get<uintptr_t>() & 0x3FFF000);

    pagetable = static_cast<PTE*>(efiAllocatePage(ptbase, ST));
    efiMapPage(pagetable, pagetable, ST);

    size_t mapSize = 0, entSize = 0;
    uint64_t mapKey = 0;
    uint32_t entVer = 0;
    EFI_MEMORY_DESCRIPTOR *map = 0, *ent;

    ST->BootServices->GetMemoryMap(&mapSize, map, &mapKey, &entSize, &entVer);
    map = static_cast<EFI_MEMORY_DESCRIPTOR*>(alloca(mapSize));
    ST->BootServices->GetMemoryMap(&mapSize, map, &mapKey, &entSize, &entVer);
    for (ent = map;
        ent < reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(
            (uintptr_t)map + mapSize);
        ent = reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(
            (uintptr_t)ent + entSize)) {
      if (ent->Type == EFI_MEMORY_TYPE_CONVENTIONAL) continue;
      for (uintptr_t ptr = ent->PhysicalStart;
          ptr < ent->PhysicalStart + ent->NumberOfPages * 0x1000;
          ptr += 0x1000) {
        efiMapPage(pagetable, reinterpret_cast<void*>(ptr), ST);
      }
    }
    ST->BootServices->ExitBootServices(EFI::getImageHandle(), mapKey);
    asm volatile("mov %q0, %%cr3"::"r"(pagetable));
  } else {
    PTE::find((uintptr_t)0, pagetable)->present = 0;
    fillPages(0x1000, 0x3FFFF000, pagetable);

    static const size_t pdpe_num = 64;
    static const size_t ptsz = (3 + pdpe_num) * 0x1000;

    uintptr_t ptbase = 0x600000 - ptsz + ((RAND::get<uintptr_t>() & 0x3FFF) << 12);

    PTE *newpt = reinterpret_cast<PTE*>(ptbase);
    Memory::fill(newpt, 0, ptsz);
    newpt[0] = PTE { ptbase + 0x1000, 3 };
    PTE *pde = newpt->getPTE();
    pde[0] = PTE { ptbase + 0x2000, 3 };
    PTE *pdpe = pde->getPTE();
    for (size_t i = 0; i < pdpe_num; i++) {
      pdpe[i] = PTE { ptbase + (3+i) * 0x1000, 3 };
    }

    fillPages(ptbase, ptbase + ptsz, newpt);
    fillPages(0x09F000, 0x0A0000, newpt);  // Extended BIOS Data
    fillPages(0x0A0000, 0x0C8000, newpt);  // Video data & VGA BIOS
    fillPages(0x0C8000, 0x0F0000, newpt);  // Reserved for many systems
    fillPages(0x0F0000, 0x100000, newpt);  // BIOS Code

    fillPages(stack_start, stack_top, newpt);  // PXOS Stack
    fillPages(text_start, data_top, newpt);    // PXOS Code & Data
    fillPages(bss_start, bss_top, newpt);      // PXOS BSS

    if (multiboot)
      *PTE::find(multiboot, newpt) = PTE { multiboot, 3 };

    pagetable = newpt;
    asm volatile("mov %q0, %%cr3"::"r"(pagetable));
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
