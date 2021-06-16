//    PhoeniX OS Kernel library standard functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

namespace klib {
  template<typename T> inline static T __attribute__((pure)) max(T a, T b) { return a > b ? a : b; }
  template<typename T> inline static T __attribute__((pure)) min(T a, T b) { return a < b ? a : b; }
  template<typename T> inline static T __attribute__((pure)) abs(T a) { return a > 0 ? a : -a; }

  size_t strlen(const char*, size_t limit = static_cast<size_t>(-1)) __attribute__((pure));
  int strncmp(const char*, const char*, int) __attribute__((pure));
  int strcmp(const char*, const char*) __attribute__((pure));
  void puts(const char *str);

  inline static uint64_t rdtsc() {
    uint32_t eax, edx;
    asm volatile("rdtsc":"=a"(eax), "=d"(edx));
    return ((uint64_t(edx) << 32) | eax);
  }

  inline static uintptr_t __align(uintptr_t base, size_t align) {
    if (base % align == 0)
      return base;
    return base + align - (base % align);
  }
}  // namespace klib
