//    PhoeniX OS Kernel library standard functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

#define PACKED __attribute__((packed))
#define NORETURN __attribute__((noreturn))
#define PURE __attribute__((pure))
#define CONST __attribute__((const))

#define alloca(size) __builtin_alloca((size))

namespace klib {
  template<typename T> inline static T PURE max(T a, T b) { return a > b ? a : b; }
  template<typename T> inline static T PURE min(T a, T b) { return a < b ? a : b; }
  template<typename T> inline static T PURE abs(T a) { return a > 0 ? a : -a; }

  size_t strlen(const char*, size_t limit = -1) PURE;
  char* strdup(const char*);
  int strcmp(const char*, const char*) PURE;

  inline static uintptr_t __align(uintptr_t base, size_t align) {
    if (base % align == 0)
      return base;
    return base + align - (base % align);
  }
}  // namespace klib
