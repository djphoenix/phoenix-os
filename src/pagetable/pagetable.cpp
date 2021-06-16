//    PhoeniX OS Memory subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "pagetable.hpp"
#include "efi.hpp"
#include "multiboot_info.hpp"

#include "kernlib/mem.hpp"
#include "kernlib/rand.hpp"

using PTE = Pagetable::Entry;

Mutex Pagetable::page_mutex;
uintptr_t Pagetable::last_page = 0x1000;
void *Pagetable::rsvd_pages[rsvd_num] {};
size_t Pagetable::rsvd_r = 0, Pagetable::rsvd_w = 0;
uintptr_t Pagetable::max_page;

static void *efiAllocatePages(uintptr_t min, size_t count, const struct EFI::SystemTable_t *ST) {
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
  Memory::zero_aligned(ptr, count * 0x1000);
  return ptr;
}

static void efiMapPage(PTE *pagetable, const void *page,
                       const struct EFI::SystemTable_t *ST, Pagetable::MemoryType type) {
  uintptr_t ptr = uintptr_t(page), min = uintptr_t(pagetable);
  uint64_t ptx = (ptr >> (12 + 9*3)) & 0x1FF;
  uint64_t pdx = (ptr >> (12 + 9*2)) & 0x1FF;
  uint64_t pdpx = (ptr >> (12 + 9)) & 0x1FF;
  uint64_t pml4x = (ptr >> 12) & 0x1FF;

  PTE *pte = pagetable + ptx;
  if (!pte->present) { [[unlikely]]
    *pte = PTE(efiAllocatePages(min, 1, ST), Pagetable::MemoryType::TABLE);
    efiMapPage(pagetable, pte->getPtr(), ST, Pagetable::MemoryType::TABLE);
  }
  PTE *pde = pte->getPTE() + pdx;
  if (!pde->present) { [[unlikely]]
    *pde = PTE(efiAllocatePages(min, 1, ST), Pagetable::MemoryType::TABLE);
    efiMapPage(pagetable, pde->getPtr(), ST, Pagetable::MemoryType::TABLE);
  }
  PTE *pdpe = pde->getPTE() + pdpx;
  if (!pdpe->present) { [[unlikely]]
    *pdpe = PTE(efiAllocatePages(min, 1, ST), Pagetable::MemoryType::TABLE);
    efiMapPage(pagetable, pdpe->getPtr(), ST, Pagetable::MemoryType::TABLE);
  }
  PTE *pml4e = pdpe->getPTE() + pml4x;
  *pml4e = PTE(page, type);
}

static void fillPagesEfi(uintptr_t low, uintptr_t top, PTE *pagetable,
                         const struct EFI::SystemTable_t *ST, Pagetable::MemoryType type) {
  low &= 0xFFFFFFFFFFFFF000;
  top = klib::__align(top, 0x1000);
  for (; low < top; low += 0x1000) {
    efiMapPage(pagetable, reinterpret_cast<void*>(low), ST, type);
  }
}

static inline void fillPagesEfi(const void *low, const void *top, PTE *pagetable,
                                const struct EFI::SystemTable_t *ST, Pagetable::MemoryType type) {
  fillPagesEfi(uintptr_t(low), uintptr_t(top), pagetable, ST, type);
}

extern "C" {
  extern const uint8_t __text_start__[], __text_end__[];
  extern const uint8_t __rodata_start__[], __rodata_end__[];
  extern const uint8_t __data_start__[], __data_end__[];
  extern const uint8_t __bss_start__[], __bss_end__[];
  extern uintptr_t __VTABLE_START__[], __VTABLE_END__[];
}

static inline __attribute__((always_inline)) void newkern_reloc(uintptr_t kernbase) {
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

void __attribute__((always_inline)) Pagetable::initEFI(const EFI::SystemTable_t *ST, Entry **newpt, uint8_t **newbase) {
  const size_t kernsz = size_t(__bss_end__ - __text_start__);

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
    max_page = klib::max(max_page, uintptr_t((ent->PhysicalStart >> 12) + ent->NumberOfPages));
  }
  ST->BootServices->FreePool(map); map = nullptr;

  uintptr_t ptbase = RAND::get<uintptr_t>(0x100, max_page) << 12;
  *newpt = static_cast<PTE*>(efiAllocatePages(ptbase, 1, ST));
  ptbase = uintptr_t(*newpt);

  ST->BootServices->GetMemoryMap(&mapSize, map, &mapKey, &entSize, &entVer);
  mapSize += 3 * entSize;
  ST->BootServices->AllocatePool(EFI::MEMORY_TYPE_DATA, mapSize, reinterpret_cast<void**>(&map));
  ST->BootServices->GetMemoryMap(&mapSize, map, &mapKey, &entSize, &entVer);

  for (EFI::MemoryDescriptor *ent = map;
      ent < reinterpret_cast<EFI::MemoryDescriptor*>(uintptr_t(map) + mapSize);
      ent = reinterpret_cast<EFI::MemoryDescriptor*>(uintptr_t(ent) + entSize)) {
    switch (ent->Type) {
      case EFI::MEMORY_TYPE_BS_CODE:
      case EFI::MEMORY_TYPE_RS_CODE:
        fillPagesEfi(ent->PhysicalStart, ent->PhysicalStart + (ent->NumberOfPages << 12), *newpt, ST, MemoryType::CODE_RX);
        break;

      case EFI::MEMORY_TYPE_BS_DATA:
      case EFI::MEMORY_TYPE_RS_DATA:

      case EFI::MEMORY_TYPE_RESERVED:
      case EFI::MEMORY_TYPE_ACPI_RECLAIM:
      case EFI::MEMORY_TYPE_ACPI_NVS:
      case EFI::MEMORY_TYPE_MAPPED_IO:
      case EFI::MEMORY_TYPE_MAPPED_IO_PORTSPACE:
        fillPagesEfi(ent->PhysicalStart, ent->PhysicalStart + (ent->NumberOfPages << 12), *newpt, ST, MemoryType::DATA_RO);
        break;
      default:
        break;
    }
  }
  ST->BootServices->FreePool(map);

  efiMapPage(*newpt, nullptr, ST, MemoryType::NULLPAGE);
  efiMapPage(*newpt, *newpt, ST, MemoryType::TABLE);

  const uint8_t *rsp; asm volatile("mov %%rsp, %q0":"=r"(rsp));
  efiMapPage(*newpt, rsp, ST, MemoryType::DATA_RW);
  efiMapPage(*newpt, rsp - 0x1000, ST, MemoryType::DATA_RW);

  DTREG gdtreg = { 0, nullptr };
  asm volatile("sgdtq %0"::"m"(gdtreg));
  efiMapPage(*newpt, gdtreg.addr, ST, MemoryType::DATA_RO);

  ST->BootServices->AllocatePages(EFI::ALLOCATE_TYPE_ANY, EFI::MEMORY_TYPE_DATA, rsvd_num, &rsvd_pages[0]);
  Memory::zero_aligned(rsvd_pages[0], rsvd_num * 0x1000);
  for (size_t i = 0; i < rsvd_num; i++) {
    rsvd_pages[i] = reinterpret_cast<uint8_t*>(rsvd_pages[0]) + i * 0x1000;
    efiMapPage(*newpt, rsvd_pages[i], ST, MemoryType::TABLE);
  }
  rsvd_w = rsvd_num - 1;

  const EFI::Framebuffer *fb = EFI::getFramebuffer();
  if (fb && fb->base) [[likely]] {
    fillPagesEfi(fb->base, static_cast<uint8_t*>(fb->base) + fb->width * fb->height * 4, *newpt, ST, MemoryType::DATA_RW);
  }

#if KERNEL_NOASLR
  *newbase = const_cast<uint8_t*>(__text_start__);
#else
  *newbase = static_cast<uint8_t*>(efiAllocatePages(RAND::get<uintptr_t>(0x100, max_page - (kernsz + 0xFFF) / 0x1000 - 1) << 12, (kernsz + 0xFFF) / 0x1000, ST));
#endif

  Memory::copy_aligned(*newbase, __text_start__, kernsz);

  ST->BootServices->ExitBootServices(EFI::getImageHandle(), mapKey);
}

void __attribute__((always_inline)) Pagetable::initMB(Entry **newpt, uint8_t **newbase) {
  const size_t kernsz = size_t(__bss_end__ - __text_start__);

  PTE *pagetable; asm volatile("mov %%cr3, %q0":"=r"(pagetable));
  PTE::find(nullptr, pagetable)->present = 0;

  static constexpr size_t pdpe_num = 64;
  static const size_t ptsz = (3 + pdpe_num + rsvd_num) * 0x1000;

  uintptr_t ptbase = RAND::get<uintptr_t>(0x800, 0x8000 - (3 + pdpe_num + rsvd_num)) << 12;
  for (size_t i = 0; i < rsvd_num; i++) {
    rsvd_pages[i] = reinterpret_cast<void*>(ptbase + (3 + pdpe_num + i) * 0x1000);
  }
  rsvd_w = rsvd_num - 1;

  *newpt = reinterpret_cast<PTE*>(ptbase);
  Memory::zero_aligned(*newpt, ptsz);
  (*newpt)[0] = PTE { ptbase + 0x1000, MemoryType::TABLE };
  PTE *pde = (*newpt)->getPTE();
  pde[0] = PTE { ptbase + 0x2000, MemoryType::TABLE };
  PTE *pdpe = pde->getPTE();
  for (size_t i = 0; i < pdpe_num; i++) {
    pdpe[i] = PTE { ptbase + (3+i) * 0x1000, MemoryType::TABLE };
  }

  _map(reinterpret_cast<uint8_t*>(ptbase), reinterpret_cast<uint8_t*>(ptbase) + ptsz, MemoryType::TABLE, *newpt);
  // 0x009F000 - 0x00A0000: Extended BIOS Data
  // 0x00A0000 - 0x00C8000: Video data & VGA BIOS
  // 0x00C8000 - 0x00F0000: Reserved for many systems
  // 0x00F0000 - 0x0100000: BIOS Code
  _map(reinterpret_cast<uint8_t*>(0x009F000), reinterpret_cast<uint8_t*>(0x0100000), MemoryType::DATA_RO, *newpt);
  _map(reinterpret_cast<uint8_t*>(0x0F00000), reinterpret_cast<uint8_t*>(0x1000000), MemoryType::DATA_RO, *newpt);  // Memory hole

  static const size_t stack_size = 0x1000;
  const uint8_t *rsp; asm volatile("mov %%rsp, %q0":"=r"(rsp));
  rsp = reinterpret_cast<uint8_t*>(klib::__align(uintptr_t(rsp), 0x1000));
  _map(rsp - stack_size, rsp, MemoryType::DATA_RW, *newpt);

#if KERNEL_NOASLR
  *newbase = const_cast<uint8_t*>(__text_start__);
#else
  *newbase = reinterpret_cast<uint8_t*>(RAND::get<uintptr_t>(
      ((ptbase + (3 + pdpe_num)) >> 12),
      0x8000 - (kernsz + 0x80000 + 0xFFF) / 0x1000) << 12);
#endif
  Memory::copy_aligned(*newbase, __text_start__, kernsz + 0x80000);
}

void Pagetable::init() {
  const EFI::SystemTable_t *ST = EFI::getSystemTable();
  Multiboot::Payload *multiboot = Multiboot::getPayload();

  const uint8_t *defbase;
  uint8_t *newbase = nullptr;
  PTE *newpt;
  ptrdiff_t kernreloc;

  asm volatile("mov $__text_start__, %q0":"=r"(defbase));

  // Initialization of pagetables

  if (ST) {
    initEFI(ST, &newpt, &newbase);
  } else {
    initMB(&newpt, &newbase);
  }

  newkern_reloc(uintptr_t(newbase));
  kernreloc = newbase - __text_start__;

  _map(__text_start__ + kernreloc, __text_end__ + kernreloc, MemoryType::CODE_RX, newpt);
  _map(__rodata_start__ + kernreloc, __rodata_end__ + kernreloc, MemoryType::DATA_RO, newpt);
  _map(__data_start__ + kernreloc, __bss_end__ + 0x80000 + kernreloc, MemoryType::DATA_RW, newpt);

  kernreloc = newbase - defbase;

  asm volatile(
      "addq $1f, %%rax;"
      "jmpq *%%rax;1:"
      "mov %q1, %%cr3;"
      :"+a"(kernreloc):"r"(newpt));

  kernreloc = reinterpret_cast<uint8_t*>(newbase) - __text_start__;
  newkern_relocstack(kernreloc);

  if (multiboot) {
    _map(multiboot, multiboot + 1, MemoryType::DATA_RW, newpt);

    if (multiboot->flags & Multiboot::FLAG_MEM) {
      max_page = ((multiboot->mem.upper * 1024) + 0x100000lu) >> 12;
    }
    if (multiboot->flags & Multiboot::FLAG_CMDLINE) {
      if (multiboot->pcmdline < 0x80000)
        multiboot->pcmdline = uint32_t(uintptr_t(__bss_end__) + multiboot->pcmdline);
      _map(reinterpret_cast<void*>(uintptr_t(multiboot->pcmdline)), reinterpret_cast<void*>(uintptr_t(multiboot->pcmdline + 0x1000)), MemoryType::DATA_RW, newpt);
    }
    if (multiboot->flags & Multiboot::FLAG_MODS) {
      if (multiboot->mods.paddr < 0x80000)
        multiboot->mods.paddr = uint32_t(uintptr_t(__bss_end__) + multiboot->mods.paddr);

      uintptr_t low = uintptr_t(multiboot->mods.paddr) & 0xFFFFFFFFFFFFF000;
      uintptr_t top = klib::__align(
          uintptr_t(multiboot->mods.paddr) +
          multiboot->mods.count * sizeof(Multiboot::Module),
          0x1000);
      _map(reinterpret_cast<void*>(low), reinterpret_cast<void*>(top), MemoryType::DATA_RW, newpt);

      const Multiboot::Module *mods =
          reinterpret_cast<Multiboot::Module*>(uintptr_t(multiboot->mods.paddr));
      for (uint32_t i = 0; i < multiboot->mods.count; i++) {
        uintptr_t low = mods[i].start;
        uintptr_t top = klib::__align(mods[i].end, 0x1000);
        _map(reinterpret_cast<void*>(low), reinterpret_cast<void*>(top), MemoryType::DATA_RW, newpt);
      }
    }

    if (multiboot->flags & Multiboot::FLAG_MEMMAP) {
      if (multiboot->mmap.paddr < 0x80000)
        multiboot->mmap.paddr = uint32_t(uintptr_t(__bss_end__) + multiboot->mmap.paddr);

      max_page = 0;
      uint8_t *mmap = reinterpret_cast<uint8_t*>(uintptr_t(multiboot->mmap.paddr));
      uint8_t *mmap_top = mmap + multiboot->mmap.size;
      while (mmap < mmap_top) {
        Multiboot::MmapEnt *ent = reinterpret_cast<Multiboot::MmapEnt*>(mmap);
        _map(ent, ent + 1, MemoryType::DATA_RO, newpt);
        uintptr_t low = uintptr_t(ent->base) & 0xFFFFFFFFFFFFF000;
        uintptr_t top = klib::__align(uintptr_t(ent->base) + ent->length, 0x1000);
        if (ent->type != Multiboot::MemoryType::MEMORY_AVAILABLE) {
          _map(reinterpret_cast<void*>(low), reinterpret_cast<void*>(top), MemoryType::DATA_RO, newpt);
        } else {
          max_page = klib::max(max_page, top >> 12);
        }
        mmap += ent->size + sizeof(ent->size);
      }
    }
    if (multiboot->flags & Multiboot::FLAG_VBETAB) {
      if (multiboot->vbe.pcontrol_info < 0x80000)
        multiboot->vbe.pcontrol_info = uint32_t(uintptr_t(__bss_end__) + multiboot->vbe.pcontrol_info);
      if (multiboot->vbe.pmode_info < 0x80000)
        multiboot->vbe.pmode_info = uint32_t(uintptr_t(__bss_end__) + multiboot->vbe.pmode_info);
      Multiboot::VBEInfo *vbe = reinterpret_cast<Multiboot::VBEInfo*>(multiboot->vbe.pcontrol_info);
      _map(vbe, vbe + 1, MemoryType::DATA_RO, newpt);
      uint8_t *vendor = reinterpret_cast<uint8_t*>(vbe->vendor_string);
      _map(vendor, vendor + 1, MemoryType::DATA_RO, newpt);
      Multiboot::VBEModeInfo *mode = reinterpret_cast<Multiboot::VBEModeInfo*>(multiboot->vbe.pmode_info);
      _map(mode, mode + 1, MemoryType::DATA_RO, newpt);
    }
  }
  if (max_page == 0) max_page = 0xfffffffffllu;
  _renewRsvd(newpt);
}

void* Pagetable::_getRsvd() {
  if (rsvd_r == rsvd_w) [[unlikely]] {
    klib::puts("OUT OF RSVD\n"); for (;;);
    return nullptr;
  }
  void *addr = rsvd_pages[rsvd_r];
  rsvd_pages[rsvd_r] = nullptr;
  rsvd_r = (rsvd_r + 1) % rsvd_num;
  return addr;
}

inline void __attribute__((always_inline)) Pagetable::_renewRsvd(PTE *pagetable) {
  size_t target_w = (rsvd_r + rsvd_num - 1) % rsvd_num;
  while (rsvd_w != target_w) [[unlikely]] {
    rsvd_w = (rsvd_w + 1) % rsvd_num;
    rsvd_pages[rsvd_w] = _alloc(false, 1, MemoryType::TABLE, pagetable);
  }
}

void* Pagetable::_map(const void* low, const void* top, MemoryType type, PTE *pagetable) {
  PTE newent { low, type };
  uintptr_t itop = (uintptr_t(top) + 0xFFF) >> 12;

  PTE *pte = nullptr, *pde = nullptr, *pdpe = nullptr, *pml4e = nullptr;

  uintptr_t ptx = (newent._ptr >> 27) & 0x1FF;
  uintptr_t pdx = (newent._ptr >> 18) & 0x1FF;
  uintptr_t pdpx = (newent._ptr >> 9) & 0x1FF;
  uintptr_t pml4x = newent._ptr & 0x1FF;

  pte = pagetable + ptx;

  for (; newent._ptr < itop;) {
    if (!pte->present) [[unlikely]] *pte = PTE(_getRsvd(), MemoryType::TABLE);

    if (pde == nullptr) [[unlikely]] pde = pte->getPTE() + pdx;
    if (!pde->present) *pde = PTE(_getRsvd(), MemoryType::TABLE);

    if (pdpe == nullptr) [[unlikely]] pdpe = pde->getPTE() + pdpx;
    if (!pdpe->present) *pdpe = PTE(_getRsvd(), MemoryType::TABLE);

    if (pml4e == nullptr) [[unlikely]] pml4e = pdpe->getPTE() + pml4x;
    *pml4e = newent;

    if (rsvd_r == rsvd_w) _renewRsvd(pagetable);

    ++newent._ptr; ++pml4x; ++pml4e;
    if (pml4x == 0x200) [[unlikely]] { pml4e = nullptr; pml4x = 0; ++pdpx; ++pdpe; } else continue;
    if (pdpx == 0x200) [[unlikely]] { pdpe = nullptr; pdpx = 0; ++pdx; ++pde; } else continue;
    if (pdx == 0x200) [[unlikely]] { pde = nullptr; pdx = 0; ++ptx; ++pte; } else continue;
  }

  asm volatile("mov %%cr3, %%rax; mov %%rax, %%cr3":::"rax");

  return const_cast<void*>(low);
}

void* Pagetable::map(const void* low, const void* top, MemoryType type) {
  Mutex::CriticalLock lock(page_mutex);
  PTE *pagetable; asm volatile("mov %%cr3, %q0":"=r"(pagetable));
  void *addr = _map(low, top, type, pagetable);
  _renewRsvd(pagetable);
  return addr;
}

void* Pagetable::map(const void* mem, MemoryType type) {
  return map(mem, reinterpret_cast<const uint8_t*>(mem) + 0x1000, type);
}

void* Pagetable::_alloc(bool low, size_t count, MemoryType type, PTE *pagetable) {
  uint8_t *addr = nullptr;
  uintptr_t i, ii;
  i = low ? 1 : last_page;
  for (;;) {
    if (i + count >= max_page) return nullptr;
    addr = reinterpret_cast<uint8_t*>(i << 12);
    bool found = 1;
    for (ii = 0; ii < count; ii++) {
      PTE *page = PTE::find(addr + (ii << 12), pagetable);
      if (page && page->present) {
        found = 0; i += ii; break;
      }
    }
    if (found) break;
    i++;
  }
  if (!low) last_page = i;

  _map(addr, addr + (count << 12), type, pagetable);
  Memory::zero_aligned(addr, count * 0x1000);
  return addr;
}

void* Pagetable::alloc(size_t count, MemoryType type) {
  Mutex::CriticalLock lock(page_mutex);
  PTE *pagetable; asm volatile("mov %%cr3, %q0":"=r"(pagetable));
  void *addr = _alloc(false, count, type, pagetable);
  _renewRsvd(pagetable);
  return addr;
}

void* Pagetable::lowalloc(size_t count, MemoryType type) {
  Mutex::CriticalLock lock(page_mutex);
  PTE *pagetable; asm volatile("mov %%cr3, %q0":"=r"(pagetable));
  void *addr = _alloc(true, count, type, pagetable);
  _renewRsvd(pagetable);
  return addr;
}

void Pagetable::free(void* page) {
  PTE *pagetable; asm volatile("mov %%cr3, %q0":"=r"(pagetable));
  Mutex::CriticalLock lock(page_mutex);
  PTE *pdata = PTE::find(page, pagetable);
  if ((pdata != nullptr) && pdata->present) {
    pdata->present = 0;
    void *addr = pdata->getPtr();
    if ((uintptr_t(addr) >> 12) < last_page && (uintptr_t(addr) >> 12) >= 0x1000)
      last_page = uintptr_t(addr) >> 12;
  }
  asm volatile("mov %%cr3, %%rax; mov %%rax, %%cr3":::"rax");
}
