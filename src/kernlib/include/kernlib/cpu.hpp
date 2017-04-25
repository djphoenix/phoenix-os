//    PhoeniX OS Kernel library cpu functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "std.hpp"

inline static uint64_t rdtsc() {
  uint32_t eax, edx;
  asm volatile("rdtsc":"=a"(eax), "=d"(edx));
  return (((uint64_t)edx << 32) | eax);
}

inline static uint64_t __attribute__((always_inline)) EnterCritical() {
  uint64_t flags;
  asm volatile("pushfq; cli; pop %q0":"=r"(flags));
  return flags;
}

inline static void LeaveCritical(uint64_t flags) {
  asm volatile("push %q0; popfq"::"r"(flags));
}
