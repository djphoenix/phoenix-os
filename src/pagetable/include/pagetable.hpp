//    PhoeniX OS Pagetable subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "mutex.hpp"
#include "efi.hpp"

struct DTREG {
  uint16_t limit;
  void* addr;
} __attribute__((packed));

class Pagetable {
 public:
  enum class MemoryType : uint8_t {
    FLAG_W = 1,
    FLAG_X = 2,
    FLAG_U = 4,

    DATA_RO = 0,
    DATA_RW = FLAG_W,
    CODE_RX = FLAG_X,
    CODE_RW = FLAG_X | FLAG_W,

    USER_DATA_RO = FLAG_U | DATA_RO,
    USER_DATA_RW = FLAG_U | DATA_RW,
    USER_CODE_RX = FLAG_U | CODE_RX,
    USER_CODE_RW = FLAG_U | CODE_RW,
    USER_TABLE = USER_CODE_RW,

    TABLE = CODE_RW,
    NULLPAGE = 8,
  };
  friend constexpr bool operator&(const MemoryType &a, const MemoryType &b) { return (uint8_t(a) & uint8_t(b)) != 0; }
  struct Entry {
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
      } __attribute__((packed));
      uint8_t flags;
    } __attribute__((packed));
    bool rsvd:1;
    uint8_t avl :3;
    uintptr_t _ptr :51;
    uint8_t nx :1;

    inline uintptr_t getUintPtr() const { return _ptr << 12; }
    inline void *getPtr() const { return reinterpret_cast<void*>(getUintPtr()); }
    inline Entry *getPTE() const { return static_cast<Entry*>(getPtr()); }

    constexpr Entry(): flags(0), rsvd(0), avl(0), _ptr(0), nx(0) {}
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wconversion"
    constexpr Entry(uintptr_t ptr, uint8_t avl, MemoryType type):
      flags(
        ((type & MemoryType::NULLPAGE) ? 0 : 1) |
        ((type & MemoryType::FLAG_W) ? 2 : 0) |
        ((type & MemoryType::FLAG_U) ? 4 : 0)
      ),
      rsvd(0), avl(avl), _ptr(ptr >> 12),
      nx((type & MemoryType::FLAG_X) ? 0 : 1) {}
    #pragma GCC diagnostic pop
    constexpr Entry(uintptr_t ptr, MemoryType type): Entry(ptr, 0, type) {}
    Entry(const void *ptr, uint8_t avl, MemoryType type): Entry(uintptr_t(ptr), avl, type) {}
    Entry(const void *ptr, MemoryType type): Entry(ptr, 0, type) {}

    static inline Entry* find(uintptr_t ptr, Entry *pagetable) {
      uintptr_t ptx   = (ptr >> (12 + 9 + 9 + 9)) & 0x1FF;
      uintptr_t pdx   = (ptr >> (12 + 9 + 9    )) & 0x1FF;
      uintptr_t pdpx  = (ptr >> (12 + 9        )) & 0x1FF;
      uintptr_t pml4x = (ptr >> (12            )) & 0x1FF;

      Entry *pt = pagetable + ptx;

      if (!pt->present) return nullptr;
      pt = pt->getPTE() + pdx;
      if (!pt->present) return nullptr;
      pt = pt->getPTE() + pdpx;
      if (!pt->present) return nullptr;
      pt = pt->getPTE() + pml4x;

      return pt;
    }
    static inline Entry* find(const void *addr, Entry *pagetable) {
      return find(uintptr_t(addr), pagetable);
    }
  } __attribute__((packed));

 private:
  static const size_t rsvd_num = 16;
  static uintptr_t max_page;
  static void* rsvd_pages[rsvd_num];
  static size_t rsvd_r, rsvd_w;
  static Mutex page_mutex;
  static uintptr_t last_page;
  static void* _alloc(bool low, size_t count, MemoryType type, Entry *pagetable);
  static void* _map(const void* low, const void* top, MemoryType type, Entry *pagetable);
  static void* _getRsvd();
  static void _renewRsvd(Entry *pagetable);
  static inline void initMB(Entry **pagetable, uint8_t **newbase);
  static inline void initEFI(const EFI::SystemTable_t *ST, Entry **pagetable, uint8_t **newbase);
  static void init();

 public:
  static void* map(const void* low, const void* top, MemoryType type);
  static void* map(const void* mem, MemoryType type);
  static void* alloc(size_t count, MemoryType type);
  static void* lowalloc(size_t count, MemoryType type);
  static void free(void* page);
};
