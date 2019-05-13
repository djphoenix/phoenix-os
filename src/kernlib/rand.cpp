//    PhoeniX OS Kernel library printf functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "kernlib.hpp"
#include "kernlib/rand.hpp"

static inline bool check_rdrand() {
  uint32_t cpuid;
  asm volatile("cpuid":"=c"(cpuid):"a"(uint32_t(1)), "c"(uint32_t(0)):"ebx", "edx");
  return cpuid & (1 << 30);
}

static uint64_t _genseed() {
  if (check_rdrand()) {
    uint64_t val;
    asm volatile("rdrandq %q0":"=r"(val));
    return val;
  }
  return klib::rdtsc();
}

static uint64_t seed = _genseed();

uint64_t RAND::_get64() {
  uint64_t x = seed;
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  seed = x;
  return x * 0x2545f4914f6cdd1dllu;
}
