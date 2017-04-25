//    PhoeniX OS Heap subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

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
};

#define ALIGNED_NEW(align) \
    void *operator new(size_t size) { return Heap::alloc(size, align); }
#define ALIGNED_NEWARR(align) \
    void *operator new[](size_t size) { return Heap::alloc(size, align); }

void *operator new(size_t, size_t);
