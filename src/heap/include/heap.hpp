//    PhoeniX OS Heap subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "mutex.hpp"

class Heap {
 private:
  struct Alloc;
  struct AllocTable;
  struct HeapPage;

  static AllocTable *allocs;
  static HeapPage *heap_pages;
  static Mutex heap_mutex;

 public:
  static void* alloc(size_t size, size_t align = 16);
  static void* realloc(void *addr, size_t size, size_t align = 16);

  static void free(void* addr);
};

template<typename T> class ptr {
 private:
  T *value;
 public:
  constexpr ptr() : value(nullptr) {}
  explicit constexpr ptr(T *value) : value(value) {}
  ~ptr() { delete value; }

  inline void reset(T* newvalue) { delete value; value = newvalue; }
  inline void operator=(T* newvalue) { reset(newvalue); }

  inline operator bool() const { return value != nullptr; }
  inline T& operator*() { return *value; }
  inline T* operator->() { return value; }
  inline T* get() { return value; }
  inline T* release() { T *val = value; value = nullptr; return val; }
  inline T& operator[](ptrdiff_t idx) { return value[idx]; }
  inline const T& operator*() const { return *value; }
  inline const T* operator->() const { return value; }
  inline const T* get() const { return value; }
  inline const T& operator[](ptrdiff_t idx) const { return value[idx]; }
};

namespace klib {
  char* strdup(const char*);
  char* strndup(const char*, size_t len);
}
