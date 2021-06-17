//    PhoeniX OS Kernel library memory functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "memop.hpp"

void Memory::copy_aligned(void *dest, const void *src, size_t count) {
  asm volatile(
      "cld;"
      "shr $3, %%rcx;"
      "cmp %%rdi, %%rsi;"
      "jae 1f;"
      "lea -8(%%rsi,%%rcx,8), %%rsi;"
      "lea -8(%%rdi,%%rcx,8), %%rdi;"
      "std;"
      "1:"
      "rep movsq;"
      :"+S"(src),"+D"(dest),"+c"(count)::"memory","cc"
  );
}

void Memory::copy(void *dest, const void *src, size_t count) {
  if (dest == src) [[unlikely]] return;
  if (
    (uintptr_t(dest) & 0x7) == 0 &&
    (uintptr_t(src) & 0x7) == 0 &&
    (
      (src >= (reinterpret_cast<uint8_t*>(dest) + count)) ||
      (dest >= (reinterpret_cast<const uint8_t*>(src) + count))
    )
  ) [[likely]] {
    size_t aligned_size = count & (~0x7ul);
    copy_aligned(dest, src, aligned_size);
    count &= 0x7;
    if (count == 0) [[likely]] return;
    dest = reinterpret_cast<uint8_t*>(dest) + aligned_size;
    src = reinterpret_cast<const uint8_t*>(src) + aligned_size;
  }
  asm volatile(
      "cld;"
      "cmp %%rdi, %%rsi;"
      "jae 1f;"
      "lea -1(%%rsi,%%rcx,1), %%rsi;"
      "lea -1(%%rdi,%%rcx,1), %%rdi;"
      "std;"
      "1:"
      "rep movsb;"
      :"+S"(src),"+D"(dest),"+c"(count)::"memory","cc"
  );
}

void Memory::zero_aligned(void *addr, size_t size) {
  asm volatile(
      "cld;"
      "xor %%rax, %%rax;"
      "shr $3, %%rcx;"
      "rep stosq;"
      :"+D"(addr),"+c"(size)::"rax","memory","cc"
  );
}

void Memory::fill_aligned(void *addr, uint8_t value, size_t size) {
  uint64_t v64 = value;
  v64 |= (v64 << 8);
  v64 |= (v64 << 16);
  v64 |= (v64 << 32);
  asm volatile(
      "cld;"
      "shr $3, %%rcx;"
      "rep stosq;"
      :"+D"(addr),"+c"(size):"a"(v64):"memory","cc"
  );
}

void Memory::zero(void *addr, size_t size) {
  if ((uintptr_t(addr) & 0x7) == 0 && (size & 0x7) == 0) [[likely]] {
    return zero_aligned(addr, size);
  }
  fill(addr, 0, size);
}

void Memory::fill(void *addr, uint8_t value, size_t size) {
  if ((uintptr_t(addr) & 0x7) == 0 && (size & 0x7) == 0) [[likely]] {
    return fill_aligned(addr, value, size);
  }
  asm volatile(
      "cld;"
      "rep stosb;"
      :"+D"(addr),"+c"(size):"a"(value):"memory","cc"
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
