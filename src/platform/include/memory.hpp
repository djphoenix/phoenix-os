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

#pragma once
#include "pxlib.hpp"
#include "multiboot_info.hpp"
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
  PTE(void *ptr, uint8_t avl, uint8_t flags):
    flags(flags), rsvd(0), avl(avl), _ptr((uintptr_t)ptr >> 12) {}
  PTE(void *ptr, uint8_t flags):
    flags(flags), rsvd(0), avl(0), _ptr((uintptr_t)ptr >> 12) {}

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
    return find((uintptr_t)addr, pagetable);
  }
} PACKED;

struct ALLOC {
  void* addr;
  size_t size;
};

struct ALLOCTABLE {
  ALLOC allocs[255];
  ALLOCTABLE* next;
  int64_t reserved;
};

class Pagetable {
  static PTE *pagetable;
  static GRUBMODULE modules[256];
  static Mutex page_mutex;
  static uint64_t last_page;
  static void* _palloc(uint8_t avl = 0, bool nolow = false);

 public:
  static void init();
  static void* map(const void* mem);
  static void* alloc(uint8_t avl = 0);
  static void free(void* page);
};

class Heap {
  static ALLOCTABLE *allocs;
  static void* first_free;
  static Mutex heap_mutex;

 public:
  static void* alloc(size_t size, size_t align = 4);
  static void* realloc(void *addr, size_t size, size_t align = 4);

  static void free(void* addr);

  template<typename T> static inline T* alloc(
      size_t size = sizeof(T), size_t align = 4) {
    return static_cast<T*>(alloc(size, align));
  }
  template<typename T> static inline T* realloc(
      T *addr, size_t size, size_t align = 4) {
    return static_cast<T*>(realloc(static_cast<void*>(addr), size, align));
  }
};

#define ALIGNED_NEW(align) \
    void *operator new(size_t size) { return Heap::alloc(size, align); }
#define ALIGNED_NEWARR(align) \
    void *operator new[](size_t size) { return Heap::alloc(size, align); }

inline static void MmioWrite32(void *p, uint32_t data) {
  Pagetable::map(p);
  *(volatile uint32_t *)(p) = data;
}
inline static uint32_t MmioRead32(const void *p) {
  Pagetable::map(p);
  return *(volatile uint32_t *)(p);
}

void *operator new(size_t, size_t);
