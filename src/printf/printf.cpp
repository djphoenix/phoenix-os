//    PhoeniX OS Kernel library printf functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "kernlib.hpp"
#include "heap.hpp"

int vprintf(const char *format, va_list ap) {
  va_list tmp;
  va_copy(tmp, ap);
  int len = vsnprintf(nullptr, 0, format, tmp);
  va_end(tmp);
  char smbuf[512], *buf;
  if (len > 511) {
    buf = static_cast<char*>(Heap::alloc(size_t(len) + 1));
  } else {
    buf = smbuf;
  }
  len = vsnprintf(buf, size_t(len), format, ap);
  buf[len] = 0;
  klib::puts(buf);
  if (len > 511) Heap::free(buf);
  return len;
}

int printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  int len = vprintf(format, args);
  va_end(args);
  return len;
}
