//    PhoeniX OS Kernel library standard functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "kernlib/std.hpp"

extern "C" {
  void puts(const char *str);
}

namespace klib {
  size_t strlen(const char* c, size_t limit) {
    const char *e = c;
    while ((size_t(e - c) < limit) && (*e++) != 0) {}
    return size_t(e - c - 1);
  }

  int strncmp(const char* a, const char* b, int max) {
    size_t i = 0;
    while (i < size_t(max) && a[i] != 0 && b[i] != 0 && a[i] == b[i]) { i++; }
    return (i < size_t(max)) ? a[i] - b[i] : 0;
  }
  int strcmp(const char* a, const char* b) {
    size_t i = 0;
    while (a[i] != 0 && b[i] != 0 && a[i] == b[i]) { i++; }
    return a[i] - b[i];
  }

  void puts(const char *str) {
    ::puts(str);
  }
}  // namespace klib
