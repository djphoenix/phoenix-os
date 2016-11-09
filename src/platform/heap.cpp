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

extern "C" {
  extern char __first_page__;
  extern PTE __pagetable__;
}

struct ALLOC {
  void* addr;
  size_t size;
};

struct ALLOCTABLE {
  ALLOC allocs[255];
  ALLOCTABLE* next;
  int64_t reserved;
};

ALLOCTABLE *Heap::allocs = 0;
void* Heap::first_free = &__first_page__;
Mutex Heap::heap_mutex;

void* Heap::alloc(size_t size, size_t align) {
  if (size == 0)
    return 0;
  heap_mutex.lock();
  uintptr_t ns = (uintptr_t)first_free, ne;
  char f;
  ALLOCTABLE *t;
  while (1) {
    if (ns % align != 0)
      ns = ns + align - (ns % align);
    ne = ns + size;
    f = 0;
    uintptr_t ps = ns >> 12, pe = (ne >> 12) + (((ne & 0xFFF) != 0) ? 1 : 0);
    for (uintptr_t i = ps; i < pe; i++) {
      // TODO: leave Heap away from pagetable
      PTE *pdata = PTE::find(i << 12, &__pagetable__);
      if ((pdata != 0) && (pdata->present) && (pdata->avl == 0)) {
        ns = (i + 1) << 12;
        if (ns % align != 0)
          ns = ns + align - (ns % align);
        ne = ns + size;
        f = 1;
      }
    }
    if (f != 0)
      continue;
    t = allocs;
    if (t == 0)
      break;
    while (1) {
      for (int i = 0; i < 255; i++) {
        if (t->allocs[i].addr == 0)
          continue;
        uintptr_t as = (uintptr_t)t->allocs[i].addr;
        uintptr_t ae = as + (uintptr_t)t->allocs[i].size;
        if (ne < as)
          continue;
        if (ae < ns)
          continue;
        if (((ns >= as) && (ns < ae)) ||  // NA starts in alloc
        ((ne >= as) && (ne < ae))
            ||  // NA ends in alloc
            ((ns >= as) && (ne < ae))
            ||  // NA inside alloc
            ((ns <= as) && (ne > ae))     // alloc inside NA
            ) {
          ns = ae;
          if (ns % align != 0)
            ns = ns + align - (ns % align);
          ne = ns + size;
          f = 1;
        }
      }
      if (f != 0)
        break;
      if (t->next == 0)
        break;
      t = t->next;
    }
    for (uintptr_t i = ps; i < pe; i++) {
      // TODO: leave Heap away from pagetable
      PTE *page = PTE::find(i << 12, &__pagetable__);
      if ((page == 0) || !page->present) {
        void *t = Pagetable::alloc(1);
        if ((uintptr_t)t != (i << 12)) {
          f = 1;
          break;
        }
      }
    }
    if (f == 0)
      break;
  }
  // Finding memory slot for alloc record
  if (allocs == 0)
    allocs = static_cast<ALLOCTABLE*>(Pagetable::alloc());
  t = allocs;
  int ai;
  while (1) {
    ai = -1;
    for (int i = 0; i < 255; i++)
      if (t->allocs[i].addr == 0) {
        ai = i;
        break;
      }
    if (ai == -1) {
      if (t->next == 0) {
        t->next = static_cast<ALLOCTABLE*>(Pagetable::alloc());
        t->next->next = 0;
      }
      t = t->next;
    } else {
      break;
    }
  }
  t->allocs[ai].addr = reinterpret_cast<void*>(ns);
  t->allocs[ai].size = size;
  if (((uintptr_t)first_free < ns) && (align == 4)) {
    first_free = reinterpret_cast<void*>(ns + size);
  }
  heap_mutex.release();
  return reinterpret_cast<void*>(ns);
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
        if ((uintptr_t)addr < (uintptr_t)first_free)
          first_free = addr;
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
