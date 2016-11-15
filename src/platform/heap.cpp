//    PhoeniX OS Heap subsystem
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

#include "heap.hpp"
#include "multiboot_info.hpp"
#include "pagetable.hpp"

struct ALLOC {
  void* addr;
  size_t size;
};

struct ALLOCTABLE {
  ALLOC allocs[255];
  ALLOCTABLE* next;
  int64_t reserved;
};

struct HEAPPAGES {
  void *pages[511];
  HEAPPAGES *next;
};

ALLOCTABLE *Heap::allocs = 0;
HEAPPAGES* Heap::heap_pages = 0;
Mutex Heap::heap_mutex;

void* Heap::alloc(size_t size, size_t align) {
  if (size == 0)
    return 0;
  heap_mutex.lock();

  if (!heap_pages)
    heap_pages = reinterpret_cast<HEAPPAGES*>(Pagetable::alloc());
  if (!allocs)
    allocs = reinterpret_cast<ALLOCTABLE*>(Pagetable::alloc());

  uintptr_t ptr = 0, ptr_top;

  HEAPPAGES *pages;
  ALLOCTABLE *table;

find_page:
  // Find page
  pages = heap_pages;
  uintptr_t page = 0xFFFFFFFFFFFFFFFF;
  while (pages) {
    for (size_t i = 0; i < 511; i++) {
      uintptr_t paddr = (uintptr_t)pages->pages[i];
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
        if (pages->pages[i] != 0) continue;
        pages->pages[i] = Pagetable::alloc();
        ptr = 0;
        goto find_page;
      }
      if (!pages->next)
        pages->next = reinterpret_cast<HEAPPAGES*>(Pagetable::alloc());
      pages = pages->next;
    }
  }
check_ptr:
  if (ptr < page) ptr = page;
  ptr = ALIGN(ptr, align);
  ptr_top = ptr + size;
  for (uintptr_t pg = ptr & 0xFFFFFFFFFFFF000; pg < ptr_top; pg += 0x1000) {
    pages = heap_pages;
    while (pages) {
      for (size_t i = 0; i < 511; i++) {
        if ((uintptr_t)pages->pages[i] == pg) goto next_page;
      }
      pages = pages->next;
    }
    pages = heap_pages;
    while (pages) {
      for (size_t i = 0; i < 511; i++) {
        if ((uintptr_t)pages->pages[i] > ptr) {
          ptr = (uintptr_t)pages->pages[i];
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
      uintptr_t alloc_base = (uintptr_t)table->allocs[i].addr;
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
      if (table->allocs[i].addr != 0) continue;
      table->allocs[i].addr = reinterpret_cast<void*>(ptr);
      table->allocs[i].size = size;
      goto done;
    }
    if (!table->next)
      table->next = reinterpret_cast<ALLOCTABLE*>(Pagetable::alloc());
    table = table->next;
  }
done:
  heap_mutex.release();

  void *a = reinterpret_cast<void*>(ptr);
  Memory::zero(a, size);

  return a;
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
