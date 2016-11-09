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

#include "kernlib.hpp"

extern "C" {
  void *__dso_handle = 0;

  void __cxa_pure_virtual() {}
  int __cxa_atexit(void (*)(void*), void*, void*) { return 0; }
}

size_t strlen(const char* c, size_t limit) {
  const char *e = c;
  while (((size_t)(e - c) < limit) && (*e++) != 0) {}
  return e - c - 1;
}

char* strdup(const char* c) {
  size_t len = strlen(c);
  char* r = new char[len + 1]();
  Memory::copy(r, c, len + 1);
  return r;
}

int strcmp(const char* a, const char* b) {
  size_t i = 0;
  while (a[i] != 0 && b[i] != 0 && a[i] == b[i]) { i++; }
  return a[i] - b[i];
}
