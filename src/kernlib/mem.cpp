//    PhoeniX OS Kernel library memory functions
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
