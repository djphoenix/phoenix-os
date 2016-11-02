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
  bool :1;
  uint8_t avl :3;
  uintptr_t _ptr :52;

  uintptr_t getUintPtr() { return _ptr << 12; }
  void *getPtr() { return reinterpret_cast<void*>(getUintPtr()); }
  PTE *getPTE() { return static_cast<PTE*>(getPtr()); }
} PACKED;
#define PTE_MAKE(ptr, flags) PTE_MAKE_AVL(ptr, 0, flags)
#define PTE_MAKE_AVL(ptr, avl, flags) (PTE) { \
    { \
  (flags & 0x01) != 0, \
  (flags & 0x02) != 0, \
  (flags & 0x04) != 0, \
  (flags & 0x08) != 0, \
  (flags & 0x10) != 0, \
  (flags & 0x20) != 0, \
  (flags & 0x40) != 0, \
  (flags & 0x80) != 0 \
    }, \
    avl, (uintptr_t)(ptr) >> 12 \
}
struct GRUBMODULE {
  uint32_t start;
  uint32_t end;
};

struct GRUB {
  uint32_t flags;
  uint32_t mem_lower, mem_upper;
  uint32_t boot_device;
  uint32_t pcmdline;
  uint32_t mods_count;
  uint32_t pmods_addr;
  uint32_t syms[3];
  uint32_t mmap_length;
  uint32_t pmmap_addr;
  uint32_t drives_length;
  uint32_t pdrives_addr;
  uint32_t pconfig_table;
  uint32_t pboot_loader_name;
  uint32_t papm_table;
  uint64_t pvbe_control_info, pvbe_mode_info, pvbe_mode, pvbe_interface_seg,
      pvbe_interface_off, pvbe_interface_len;
};
struct GRUBMEMENT {
  uint32_t size;
  void *base;
  size_t length;
  uint32_t type;
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
extern GRUB *grub_data;

class Memory {
  static PTE *pagetable;
  static ALLOCTABLE *allocs;
  static void* first_free;
  static uint64_t last_page;
  static GRUBMODULE modules[256];
  static PTE *get_page(void* base_addr);
  static Mutex page_mutex, heap_mutex;
  static void* _palloc(uint8_t avl = 0, bool nolow = false);

 public:
  static void map();
  static void init();
  static void* salloc(const void* mem);
  static void* palloc(uint8_t avl = 0);
  static void* alloc(size_t size, size_t align = 4);
  static void* realloc(void *addr, size_t size, size_t align = 4);
  static void pfree(void* page);
  static void free(void* addr);
  static void copy(void* dest, const void* src, size_t count);
  static void zero(void *addr, size_t size);
  static void fill(void *addr, uint8_t value, size_t size);

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
    void *operator new(size_t size) { return Memory::alloc(size, align); }
#define ALIGNED_NEWARR(align) \
    void *operator new[](size_t size) { return Memory::alloc(size, align); }

inline static void MmioWrite32(void *p, uint32_t data) {
  Memory::salloc(p);
  *(volatile uint32_t *)(p) = data;
}
inline static uint32_t MmioRead32(const void *p) {
  Memory::salloc(p);
  return *(volatile uint32_t *)(p);
}
