//    PhoeniX OS Kernel library cpu functions
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
