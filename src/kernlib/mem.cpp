//    PhoeniX OS Kernel library memory functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "kernlib.hpp"

void Memory::copy(void *dest, const void *src, size_t count) {
  asm volatile(
      "cld;"
      "cmp %%rdi, %%rsi;"
      "jae 1f;"
      "lea -1(%%rsi,%%rcx,1), %%rsi;"
      "lea -1(%%rdi,%%rcx,1), %%rdi;"
      "std;"
      "1:"
      "rep movsb;"
      "cld;"
      ::"S"(src),"D"(dest),"c"(count)
  );
}

void Memory::zero(void *addr, size_t size) {
  fill(addr, 0, size);
}

void Memory::fill(void *addr, uint8_t value, size_t size) {
  asm volatile(
      "cld;"
      "rep stosb;"
      ::"D"(addr),"a"(value),"c"(size)
  );
}

extern "C" {
  void memcpy(void *dest, const void *src, size_t count)
    __attribute((alias("_ZN6Memory4copyEPvPKvm")));
  void memmove(void *dest, const void *src, size_t count)
    __attribute((alias("_ZN6Memory4copyEPvPKvm")));
  void memset(void *addr, uint8_t value, size_t size)
    __attribute((alias("_ZN6Memory4fillEPvhm")));
  void bzero(void *addr, size_t size)
    __attribute((alias("_ZN6Memory4zeroEPvm")));
}
