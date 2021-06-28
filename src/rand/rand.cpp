//    PhoeniX OS Kernel library printf functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "rand.hpp"

static inline bool check_rdrand() {
  uint32_t cpuid;
  asm volatile("cpuid":"=c"(cpuid):"a"(uint32_t(1)), "c"(uint32_t(0)):"ebx", "edx");
  return (cpuid & (1 << 30)) != 0;
}

static inline bool check_rdseed() {
  uint32_t cpuid;
  asm volatile("cpuid":"=b"(cpuid):"a"(uint32_t(7)), "b"(uint32_t(0)):"ecx", "edx");
  return (cpuid & (1 << 18)) != 0;
}

void RAND::setup() {
  if (check_rdseed()) {
    asm volatile(
      "1: rdseed %q0; jnc 1b"
      :"=X"(seed)
    );
  } else if (check_rdrand()) {
    asm volatile(
      "1: rdrand %q0; jnc 1b"
      :"=X"(seed)
    );
  } else {
    uint32_t eax, edx;
    asm volatile("rdtsc":"=a"(eax), "=d"(edx));
    seed = ((uint64_t(edx) << 32) | eax);
  }
}

uint64_t RAND::seed;

uint64_t RAND::_get64() {
  uint64_t x = seed;
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  seed = x;
  return x * 0x2545f4914f6cdd1dllu;
}
