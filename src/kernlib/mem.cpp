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
      "mov %0, %%rsi;"
      "mov %1, %%rdi;"
      "cld;"
      "cmp %%rdi, %%rsi;"
      "jae 1f;"
      "add %%rcx, %%rsi; dec %%rsi;"
      "add %%rcx, %%rdi; dec %%rdi;"
      "std;"
      "\n1:"
      "rep movsb;"
      "cld;"
      ::"r"(src),"r"(dest),"c"(count):"rsi","rdi"
  );
}

void Memory::zero(void *addr, size_t size) {
  fill(addr, 0, size);
}

void Memory::fill(void *addr, uint8_t value, size_t size) {
  asm volatile(
      "mov %0, %%rdi;"
      "cld;"
      "rep stosb;"
      ::"r"(addr),"a"(value),"c"(size):"rdi"
  );
}
