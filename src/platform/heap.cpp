//    PhoeniX OS Heap subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "heap.hpp"
#include "multiboot_info.hpp"
#include "pagetable.hpp"

struct Heap::Alloc {
  void* addr;
  size_t size;
};

struct Heap::AllocTable {
  Alloc allocs[255];
  AllocTable* next;
  int64_t reserved;
};

struct Heap::HeapPage {
  void *pages[511];
  HeapPage *next;
};

Heap::AllocTable *Heap::allocs = nullptr;
Heap::HeapPage* Heap::heap_pages = nullptr;
Mutex Heap::heap_mutex;

void* Heap::alloc(size_t size, size_t align) {
  if (size == 0)
    return nullptr;
  Mutex::Lock lock(heap_mutex);

  if (!heap_pages) heap_pages = reinterpret_cast<HeapPage*>(Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW));
  if (!allocs) allocs = reinterpret_cast<AllocTable*>(Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW));

  uintptr_t ptr = 0, ptr_top;

  HeapPage *pages;
  AllocTable *table;

find_page:
  // Find page
  pages = heap_pages;
  uintptr_t page = 0xFFFFFFFFFFFFFFFF;
  while (pages) {
    for (size_t i = 0; i < 511; i++) {
      uintptr_t paddr = uintptr_t(pages->pages[i]);
      if (paddr == 0) continue;
      if (page > paddr && paddr >= ptr) {
        page = paddr;
        goto check_ptr;
      }
    }
    pages = pages->next;
  }
  if (page == 0xFFFFFFFFFFFFFFFF) {
new_page:
    // Alloc new page
    pages = heap_pages;
    while (pages) {
      for (size_t i = 0; i < 511; i++) {
        if (pages->pages[i] != nullptr) continue;
        pages->pages[i] = Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW);
        ptr = 0;
        goto find_page;
      }
      if (!pages->next)
        pages->next = reinterpret_cast<HeapPage*>(Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW));
      pages = pages->next;
    }
  }
check_ptr:
  if (ptr < page) ptr = page;
  ptr = klib::__align(ptr, align);
  ptr_top = ptr + size;
  for (uintptr_t pg = ptr & 0xFFFFFFFFFFFF000; pg < ptr_top; pg += 0x1000) {
    pages = heap_pages;
    while (pages) {
      for (size_t i = 0; i < 511; i++) {
        if (uintptr_t(pages->pages[i]) == pg) goto next_page;
      }
      pages = pages->next;
    }
    pages = heap_pages;
    while (pages) {
      for (size_t i = 0; i < 511; i++) {
        if (uintptr_t(pages->pages[i]) > ptr) {
          ptr = uintptr_t(pages->pages[i]);
          goto check_ptr;
        }
      }
      pages = pages->next;
    }
    goto new_page;
next_page:
    continue;
  }
  table = allocs;
  while (table) {
    for (size_t i = 0; i < 255; i++) {
      uintptr_t alloc_base = uintptr_t(table->allocs[i].addr);
      uintptr_t alloc_top = alloc_base + table->allocs[i].size;
      if (ptr >= alloc_top || ptr_top < alloc_base) continue;
      ptr = alloc_top;
      goto check_ptr;
    }
    table = table->next;
  }

  // Save
  table = allocs;
  while (table) {
    for (size_t i = 0; i < 255; i++) {
      if (table->allocs[i].addr != nullptr) continue;
      table->allocs[i].addr = reinterpret_cast<void*>(ptr);
      table->allocs[i].size = size;
      goto done;
    }
    if (!table->next)
      table->next = reinterpret_cast<AllocTable*>(Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW));
    table = table->next;
  }
done:

  void *a = reinterpret_cast<void*>(ptr);
  Memory::zero(a, size);

  return a;
}
void Heap::free(void* addr) {
  if (addr == nullptr) return;
  Mutex::Lock lock(heap_mutex);
  AllocTable *t = allocs;
  while (1) {
    for (int i = 0; i < 255; i++) {
      if (t->allocs[i].addr == addr) {
        t->allocs[i].addr = nullptr;
        t->allocs[i].size = 0;
        return;
      }
    }
    if (t->next == nullptr) return;
    t = t->next;
  }
}
void *Heap::realloc(void *addr, size_t size, size_t align) {
  if (size == 0) {
    free(addr);
    return nullptr;
  }
  if (addr == nullptr) return alloc(size, align);
  size_t oldsize = 0;
  {
    Mutex::Lock lock(heap_mutex);
    AllocTable *t = allocs;
    while (t != nullptr) {
      for (int i = 0; i < 255; i++) {
        if (t->allocs[i].addr == addr) {
          oldsize = t->allocs[i].size;
          if (oldsize >= size) {
            t->allocs[i].size = size;
            return addr;
          }
          break;
        }
      }
      if (oldsize != 0)
        break;
      t = t->next;
    }
  }
  if (oldsize == 0)
    return nullptr;
  void *newptr = alloc(size, align);
  Memory::copy(newptr, addr, oldsize);
  free(addr);
  return newptr;
}

void* __attribute__((always_inline)) operator new(size_t a) {
  return Heap::alloc(a);
}
void* operator new[](size_t a) __attribute__((alias("_Znwm")));
void operator delete(void* a) noexcept __attribute__((alias("_ZN4Heap4freeEPv")));
void operator delete[](void* a) noexcept __attribute__((alias("_ZN4Heap4freeEPv")));
