//    PhoeniX OS Pagetable subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"

struct PTE {
  union {
    struct {
      bool present :1;
      bool readwrite :1;
      bool user :1;
      bool writethrough :1;
      bool disablecache :1;
      bool accessed :1;
      bool dirty :1;
      bool size :1;
    } PACKED;
    uint8_t flags;
  } PACKED;
  bool rsvd:1;
  uint8_t avl :3;
  uintptr_t _ptr :52;

  uintptr_t getUintPtr() { return _ptr << 12; }
  void *getPtr() { return reinterpret_cast<void*>(getUintPtr()); }
  PTE *getPTE() { return static_cast<PTE*>(getPtr()); }

  PTE(): flags(0), rsvd(0), avl(0), _ptr(0) {}
  PTE(uintptr_t ptr, uint8_t avl, uint8_t flags):
    flags(flags), rsvd(0), avl(avl), _ptr(ptr >> 12) {}
  PTE(uintptr_t ptr, uint8_t flags):
    flags(flags), rsvd(0), avl(0), _ptr(ptr >> 12) {}
  PTE(const void *ptr, uint8_t avl, uint8_t flags):
    flags(flags), rsvd(0), avl(avl), _ptr(uintptr_t(ptr) >> 12) {}
  PTE(const void *ptr, uint8_t flags):
    flags(flags), rsvd(0), avl(0), _ptr(uintptr_t(ptr) >> 12) {}

  static PTE* find(uintptr_t ptr, PTE *pagetable) {
    uint16_t ptx = (ptr >> (12 + 9 + 9 + 9)) & 0x1FF;
    uint16_t pdx = (ptr >> (12 + 9 + 9)) & 0x1FF;
    uint16_t pdpx = (ptr >> (12 + 9)) & 0x1FF;
    uint16_t pml4x = (ptr >> (12)) & 0x1FF;

    PTE *pde = pagetable[ptx].present ? pagetable[ptx].getPTE() : 0;
    if (pde == 0) return 0;
    PTE *pdpe = pde[pdx].present ? pde[pdx].getPTE() : 0;
    if (pdpe == 0) return 0;
    PTE *page = pdpe[pdpx].present ? pdpe[pdpx].getPTE() : 0;
    if (page == 0) return 0;

    return &page[pml4x];
  }
  static PTE* find(void *addr, PTE *pagetable) {
    return find(uintptr_t(addr), pagetable);
  }
} PACKED;

class Pagetable {
  static PTE *pagetable;
  static Mutex page_mutex;
  static uint64_t last_page;
  static void* _alloc(uint8_t avl = 0, bool nolow = false);
  static void init();

 public:
  static void* map(const void* mem);
  static void* alloc(uint8_t avl = 0);
  static void free(void* page);
};

inline static void MmioWrite32(void *p, uint32_t data) {
  Pagetable::map(p);
  *reinterpret_cast<volatile uint32_t *>(p) = data;
}
inline static uint32_t MmioRead32(const void *p) {
  Pagetable::map(p);
  return *reinterpret_cast<const volatile uint32_t *>(p);
}
