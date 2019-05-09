//    PhoeniX OS Kernel library standard functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "kernlib.hpp"

extern "C" {
  void *__dso_handle = 0;

  void CONST __cxa_pure_virtual() {}
  int CONST __cxa_atexit(void (*)(void*), void*, void*) { return 0; }
}

size_t strlen(const char* c, size_t limit) {
  const char *e = c;
  while ((size_t(e - c) < limit) && (*e++) != 0) {}
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
