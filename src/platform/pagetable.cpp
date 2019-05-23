//    PhoeniX OS Memory subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "pagetable.hpp"
#include "efi.hpp"
#include "multiboot_info.hpp"
#include "interrupts.hpp"

using PTE = Pagetable::Entry;

Mutex Pagetable::page_mutex;
uintptr_t Pagetable::last_page = 1;
void *Pagetable::rsvd_pages[rsvd_num];
static uintptr_t max_page = 0xFFFFFFFFFFFFFllu;
static Mutex renewGuard;

static void fillPages(uintptr_t low, uintptr_t top, PTE *pagetable, uint8_t flags = 3) {
  low &= 0xFFFFFFFFFFFFF000llu;
  top = klib::__align(top, 0x1000);
  for (; low < top; low += 0x1000)
    *PTE::find(low, pagetable) = PTE(low, flags);
}

static inline void fillPages(const void *low, const void *top, PTE *pagetable, uint8_t flags = 3) {
  fillPages(uintptr_t(low), uintptr_t(top), pagetable, flags);
}

static void *efiAllocatePages(uintptr_t min, size_t count, const struct EFI::SystemTable *ST) {
  size_t mapSize = 0, entSize = 0;
  EFI::MemoryDescriptor *map = nullptr, *ent;
  uint64_t mapKey;
  uint32_t entVer = 0;

  void *ptr = nullptr;

  ST->BootServices->GetMemoryMap(&mapSize, map, &mapKey, &entSize, &entVer);
  mapSize += 3 * entSize;
  ST->BootServices->AllocatePool(EFI::MEMORY_TYPE_DATA, mapSize, reinterpret_cast<void**>(&map));
  ST->BootServices->GetMemoryMap(&mapSize, map, &mapKey, &entSize, &entVer);
  for (ent = map;
      ent < reinterpret_cast<EFI::MemoryDescriptor*>(uintptr_t(map) + mapSize);
      ent = reinterpret_cast<EFI::MemoryDescriptor*>(uintptr_t(ent) + entSize)) {
    if (ent->Type != EFI::MEMORY_TYPE_CONVENTIONAL) continue;
    uintptr_t end = ent->PhysicalStart + ent->NumberOfPages * 0x1000;
    if (end <= min) continue;
    if (end - klib::max(min, uintptr_t(ent->PhysicalStart)) <= count * 0x1000) continue;
    ptr = reinterpret_cast<void*>(klib::max(min, uintptr_t(ent->PhysicalStart)));
    break;
  }
  ST->BootServices->FreePool(map);
  ST->BootServices->AllocatePages(
      ptr ? EFI::ALLOCATE_TYPE_ADDR : EFI::ALLOCATE_TYPE_ANY,
          EFI::MEMORY_TYPE_DATA, count, &ptr);
  Memory::zero(ptr, count * 0x1000);
  return ptr;
}

static inline void *efiAllocatePage(uintptr_t min, const struct EFI::SystemTable *ST) {
  return efiAllocatePages(min, 1, ST);
}

static void efiMapPage(PTE *pagetable, const void *page,
                       const struct EFI::SystemTable *ST, uint8_t flags = 3) {
  uintptr_t ptr = uintptr_t(page), min = uintptr_t(pagetable);
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
  *pml4e = PTE(page, flags);
}

static void fillPagesEfi(uintptr_t low, uintptr_t top, PTE *pagetable,
                         const struct EFI::SystemTable *ST, uint8_t flags = 3) {
  low &= 0xFFFFFFFFFFFFF000;
  top = klib::__align(top, 0x1000);
  for (; low < top; low += 0x1000) {
    efiMapPage(pagetable, reinterpret_cast<void*>(low), ST, flags);
  }
}

static inline void fillPagesEfi(const void *low, const void *top, PTE *pagetable,
                                const struct EFI::SystemTable *ST, uint8_t flags = 3) {
  fillPagesEfi(uintptr_t(low), uintptr_t(top), pagetable, ST, flags);
}

static inline __attribute__((always_inline)) void newkern_reloc(uintptr_t kernbase) {
  const char *__text_start__, *__bss_end__;
  asm volatile(
      "lea __text_start__(%%rip), %q0;"
      "lea __bss_end__(%%rip), %q1;"
      :"=r"(__text_start__), "=r"(__bss_end__));
  uintptr_t *__VTABLE_START__, *__VTABLE_END__;
  asm volatile(
      "lea __VTABLE_START__(%%rip), %q0;"
      "lea __VTABLE_END__(%%rip), %q1;"
      :"=r"(__VTABLE_START__), "=r"(__VTABLE_END__));

  ptrdiff_t kernreloc = ptrdiff_t(kernbase) - ptrdiff_t(__text_start__);
  for (uintptr_t *p = __VTABLE_START__; p < __VTABLE_END__; p++) {
    if (*p < uintptr_t(__text_start__)) continue;
    if (*p > uintptr_t(__bss_end__)) continue;
    uintptr_t *np = p + kernreloc / 8;
    *np = (*p + uintptr_t(kernreloc));
  }
}

static inline __attribute__((always_inline)) void newkern_relocstack(ptrdiff_t kernreloc) {
  struct stackframe {
    struct stackframe* rbp;
    uintptr_t rip;
  } __attribute__((packed));
  struct stackframe *frame;
  asm volatile("mov %%rbp, %q0":"=r"(frame)::);
  while (frame != nullptr) {
    frame->rip += uintptr_t(kernreloc);
    frame = frame->rbp;
  }
}

static inline __attribute__((always_inline)) void newkern_reinit() {
  uintptr_t *__CTOR_LIST__; asm volatile("leaq __CTOR_LIST__(%%rip), %q0":"=r"(__CTOR_LIST__));
  for (uintptr_t *p = __CTOR_LIST__ + 1; *p != 0; p++) {
    void (*func)(void) = reinterpret_cast<void(*)(void)>(*p);
    func();
  }
}

void Pagetable::init() {
  extern const char __text_start__[], __text_end__[];
  extern const char __modules_start__[], __modules_end__[];
  extern const char __data_start__[], __data_end__[];
  extern const char __bss_start__[], __bss_end__[];

  const size_t kernsz = size_t(__bss_end__ - __text_start__);

  const struct EFI::SystemTable *ST = EFI::getSystemTable();
  Multiboot::Payload *multiboot = Multiboot::getPayload();

  // Initialization of pagetables

  if (ST) {
    EFI::LoadedImage *loaded_image = nullptr;
    ST->BootServices->HandleProtocol(
        EFI::getImageHandle(), &EFI::GUID_LoadedImageProtocol,
        reinterpret_cast<void**>(&loaded_image));

    size_t mapSize = 0, entSize = 0;
    uint64_t mapKey = 0;
    uint32_t entVer = 0;
    max_page = 0;
    EFI::MemoryDescriptor *map = nullptr;
    ST->BootServices->GetMemoryMap(&mapSize, map, &mapKey, &entSize, &entVer);
    mapSize += 3 * entSize;
    ST->BootServices->AllocatePool(EFI::MEMORY_TYPE_DATA, mapSize, reinterpret_cast<void**>(&map));
    ST->BootServices->GetMemoryMap(&mapSize, map, &mapKey, &entSize, &entVer);

    for (EFI::MemoryDescriptor *ent = map;
        ent < reinterpret_cast<EFI::MemoryDescriptor*>(uintptr_t(map) + mapSize);
        ent = reinterpret_cast<EFI::MemoryDescriptor*>(uintptr_t(ent) + entSize)) {
      max_page = klib::max(max_page, (ent->PhysicalStart >> 12) + ent->NumberOfPages);
    }
    ST->BootServices->FreePool(map);

    uintptr_t ptbase = RAND::get<uintptr_t>(0x100, max_page) << 12;
    PTE *pagetable = static_cast<PTE*>(efiAllocatePage(ptbase, ST));
    ptbase = uintptr_t(pagetable);

    efiMapPage(pagetable, nullptr, ST, 0);
    efiMapPage(pagetable, pagetable, ST);

    const char *rsp; asm volatile("mov %%rsp, %q0":"=r"(rsp));
    efiMapPage(pagetable, rsp, ST);
    efiMapPage(pagetable, rsp - 0x1000, ST);

    const EFI::Framebuffer *fb = EFI::getFramebuffer();
    if (fb && fb->base) {
      fillPagesEfi(fb->base, reinterpret_cast<char*>(fb->base) + fb->width * fb->height * 4, pagetable, ST);
    }

    char *base; asm volatile("lea __text_start__(%%rip), %q0":"=r"(base));
    void *newbase = efiAllocatePages(RAND::get<uintptr_t>(0x100, max_page) << 12, (kernsz + 0xFFF) / 0x1000, ST);

    ptrdiff_t kernreloc = ptrdiff_t(newbase) - ptrdiff_t(__text_start__);
    Memory::copy(newbase, base, kernsz);

    newkern_reloc(uintptr_t(newbase));

    fillPagesEfi(__text_start__ + kernreloc, __text_end__ + kernreloc, pagetable, ST, 3);
    fillPagesEfi(__modules_start__ + kernreloc, __modules_end__ + kernreloc, pagetable, ST, 1);
    fillPagesEfi(__data_start__ + kernreloc, __data_end__ + kernreloc, pagetable, ST);
    fillPagesEfi(__bss_start__ + kernreloc, __bss_end__ + kernreloc, pagetable, ST);

    DTREG gdtreg = { 0, nullptr };
    asm volatile("sgdtq (%q0)"::"r"(&gdtreg));
    efiMapPage(pagetable, gdtreg.addr, ST);

    void *rsvd[rsvd_num];

    for (size_t i = 0; i < rsvd_num; i++) {
      ST->BootServices->AllocatePages(EFI::ALLOCATE_TYPE_ANY, EFI::MEMORY_TYPE_DATA, 1, &rsvd[i]);
      efiMapPage(pagetable, rsvd[i], ST);
      Memory::fill(rsvd[i], 0, 0x1000);
    }

    ST->BootServices->ExitBootServices(EFI::getImageHandle(), mapKey);

    asm volatile(
        "movq %q1, %%rax;"
        "addq $1f, %%rax;"
        "jmpq *%%rax;1:"
        "mov %q0, %%cr3;"
        ::"r"(pagetable), "r"(kernreloc) : "rax");

    kernreloc = reinterpret_cast<char*>(newbase) - base;
    newkern_relocstack(kernreloc);
    newkern_reinit();

    Memory::copy(rsvd_pages, rsvd, sizeof(rsvd));
  } else {
    PTE *pagetable; asm volatile("mov %%cr3, %q0":"=r"(pagetable));
    PTE::find(nullptr, pagetable)->present = 0;
    fillPages(0x1000, 0x3FFFF000, pagetable);

    static const size_t pdpe_num = 64;
    static const size_t ptsz = (3 + pdpe_num + rsvd_num) * 0x1000;

    uintptr_t ptbase = RAND::get<uintptr_t>(0x800, 0x8000 - (3 + pdpe_num + rsvd_num)) << 12;
    for (size_t i = 0; i < rsvd_num; i++) {
      rsvd_pages[i] = reinterpret_cast<void*>(ptbase + (3 + pdpe_num + i) * 0x1000);
    }

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
    fillPages(0x009F000, 0x00A0000, newpt);  // Extended BIOS Data
    fillPages(0x00A0000, 0x00C8000, newpt);  // Video data & VGA BIOS
    fillPages(0x00C8000, 0x00F0000, newpt);  // Reserved for many systems
    fillPages(0x00F0000, 0x0100000, newpt);  // BIOS Code
    fillPages(0x0F00000, 0x1000000, newpt);  // Memory hole

    static const size_t stack_size = 0x1000;
    extern const char __stack_end__[];
    const char *__stack_start__ = __stack_end__ - stack_size;
    fillPages(__stack_start__, __stack_end__, newpt);

    uintptr_t kernbase = RAND::get<uintptr_t>(
        ((ptbase + (3 + pdpe_num)) >> 12),
        0x8000 - (kernsz + 0x80000 + 0xFFF) / 0x1000) << 12;

    ptrdiff_t kernreloc = ptrdiff_t(kernbase) - ptrdiff_t(__text_start__);
    Memory::copy(reinterpret_cast<char*>(kernbase), __text_start__, kernsz + 0x80000);

    newkern_reloc(kernbase);

    fillPages(__text_start__ + kernreloc, __text_end__ + kernreloc, newpt, 3);
    fillPages(__modules_start__ + kernreloc, __modules_end__ + kernreloc, newpt, 1);
    fillPages(__data_start__ + kernreloc, __data_end__ + kernreloc, newpt);
    fillPages(__bss_start__ + kernreloc, __bss_end__ + 0x80000 + kernreloc, newpt);

    asm volatile(
        "movq %q1, %%rax;"
        "addq $1f, %%rax;"
        "jmpq *%%rax;1:"
        "mov %q0, %%cr3;"
        ::"r"(newpt), "r"(kernreloc) : "rax");

    newkern_relocstack(kernreloc);
    newkern_reinit();
  }

  if (multiboot) {
    map(multiboot);
    uintptr_t bss_end; asm volatile("lea __bss_end__(%%rip), %q0":"=r"(bss_end));

    if (multiboot->flags & Multiboot::FLAG_MEM) {
      max_page = ((multiboot->mem.upper * 1024) + 0x100000lu) >> 12;
    }
    if (multiboot->flags & Multiboot::FLAG_CMDLINE) {
      if (multiboot->pcmdline < 0x80000)
        multiboot->pcmdline += bss_end;
      map(reinterpret_cast<void*>(uintptr_t(multiboot->pcmdline)));
    }
    if (multiboot->flags & Multiboot::FLAG_MODS) {
      if (multiboot->mods.paddr < 0x80000)
        multiboot->mods.paddr += bss_end;

      uintptr_t low = uintptr_t(multiboot->mods.paddr) & 0xFFFFFFFFFFFFF000;
      uintptr_t top = klib::__align(
          uintptr_t(multiboot->mods.paddr) +
          multiboot->mods.count * sizeof(Multiboot::Module),
          0x1000);
      for (uintptr_t addr = low; addr < top; addr += 0x1000)
        map(reinterpret_cast<void*>(addr));

      const Multiboot::Module *mods =
          reinterpret_cast<Multiboot::Module*>(uintptr_t(multiboot->mods.paddr));
      for (uint32_t i = 0; i < multiboot->mods.count; i++) {
        uintptr_t low = mods[i].start;
        uintptr_t top = klib::__align(mods[i].end, 0x1000);
        for (uintptr_t addr = low; addr < top; addr += 0x1000)
          map(reinterpret_cast<void*>(addr));
      }
    }

    if (multiboot->flags & Multiboot::FLAG_MEMMAP) {
      if (multiboot->mmap.paddr < 0x80000)
        multiboot->mmap.paddr += bss_end;

      max_page = 0;
      const char *mmap = reinterpret_cast<const char*>(uintptr_t(multiboot->mmap.paddr));
      const char *mmap_top = mmap + multiboot->mmap.size;
      while (mmap < mmap_top) {
        const Multiboot::MmapEnt *ent = reinterpret_cast<const Multiboot::MmapEnt*>(mmap);
        map(ent);
        uintptr_t low = uintptr_t(ent->base) & 0xFFFFFFFFFFFFF000;
        uintptr_t top = klib::__align(uintptr_t(ent->base) + ent->length, 0x1000);
        if (ent->type != 1) {
          for (uintptr_t addr = low; addr < top; addr += 0x1000)
            map(reinterpret_cast<void*>(addr));
        } else {
          max_page = klib::max(max_page, top >> 12);
        }
        mmap += ent->size + sizeof(ent->size);
      }
    }
  }
}

void* Pagetable::_getRsvd() {
  void *addr;
  for (size_t i = 0; i < rsvd_num; i++) {
    addr = rsvd_pages[i];
    if (addr != nullptr) {
      rsvd_pages[i] = nullptr;
      return addr;
    }
  }
  printf("OUT OF RSVD\n"); for (;;);
  return nullptr;
}

void Pagetable::_renewRsvd() {
  if (!renewGuard.try_lock()) return;
  for (size_t i = 0; i < rsvd_num; i++) {
    if (rsvd_pages[i] != nullptr) continue;
    rsvd_pages[i] = _alloc(0, true);
  }
  renewGuard.release();
}

void* Pagetable::_map(const void* mem) {
  PTE *pagetable; asm volatile("mov %%cr3, %q0":"=r"(pagetable));

  uintptr_t i = uintptr_t(mem) >> 12;
  uint16_t ptx = (i >> 27) & 0x1FF;
  uint16_t pdx = (i >> 18) & 0x1FF;
  uint16_t pdpx = (i >> 9) & 0x1FF;
  uint16_t pml4x = i & 0x1FF;
  void *addr = reinterpret_cast<void*>(i << 12);

  PTE *p = PTE::find(addr, pagetable);
  if (p && p->present) return addr;

  if (!pagetable[ptx].present) pagetable[ptx] = PTE(_getRsvd(), 3);
  PTE *pde = pagetable[ptx].getPTE();
  if (!pde[pdx].present) pde[pdx] = PTE(_getRsvd(), 3);
  PTE *pdpe = pde[pdx].getPTE();
  if (!pdpe[pdpx].present) pdpe[pdpx] = PTE(_getRsvd(), 3);
  PTE *pml4e = pdpe[pdpx].getPTE();
  pml4e[pml4x] = PTE(addr, 3);

  _renewRsvd();

  return addr;
}

void* Pagetable::map(const void* mem) {
  uint64_t t = EnterCritical();
  page_mutex.lock();
  void *addr = _map(mem);
  page_mutex.release();
  LeaveCritical(t);
  return addr;
}

void* Pagetable::_alloc(uint8_t avl, bool nolow) {
  PTE *pagetable; asm volatile("mov %%cr3, %q0":"=r"(pagetable));
  void *addr = nullptr;
  PTE *page;
  uintptr_t i = last_page - 1;
  if (nolow && (i < 0x100)) i = 0x100;
  while (i < max_page) {
    i++;
    addr = reinterpret_cast<void*>(i << 12);
    page = PTE::find(addr, pagetable);
    if ((page == nullptr) || !page->present) break;
  }
  if (!nolow) last_page = i;

  _map(addr);
  PTE::find(addr, pagetable)->avl = avl;
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
  PTE *pagetable; asm volatile("mov %%cr3, %q0":"=r"(pagetable));
  uint64_t t = EnterCritical();
  page_mutex.lock();
  PTE *pdata = PTE::find(page, pagetable);
  if ((pdata != nullptr) && pdata->present) {
    pdata->present = 0;
    void *addr = pdata->getPtr();
    if ((uintptr_t(addr) >> 12) < last_page)
      last_page = uintptr_t(addr) >> 12;
  }
  page_mutex.release();
  LeaveCritical(t);
}
