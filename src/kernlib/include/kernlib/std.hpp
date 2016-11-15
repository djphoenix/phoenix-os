//    PhoeniX OS Kernel library standard functions
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

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

template<typename T> inline static T MAX(T a, T b) { return a > b ? a : b; }
template<typename T> inline static T MIN(T a, T b) { return a < b ? a : b; }
template<typename T> inline static T ABS(T a) { return a > 0 ? a : -a; }

#define PACKED __attribute__((packed))
#define NORETURN __attribute__((noreturn))

#define alloca(size) __builtin_alloca((size))

size_t strlen(const char*, size_t limit = -1);
char* strdup(const char*);
int strcmp(const char*, const char*);

inline static uintptr_t ALIGN(uintptr_t base, size_t align) {
  if (base % align == 0)
    return base;
  return base + align - (base % align);
}
