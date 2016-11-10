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

#pragma once
#include "kernlib.hpp"

struct ALLOCTABLE;
struct HEAPPAGES;

class Heap {
  static ALLOCTABLE *allocs;
  static HEAPPAGES *heap_pages;
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

void *operator new(size_t, size_t);
