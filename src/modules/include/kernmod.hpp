//    PhoeniX OS module includes
//    Copyright © 2019 Yury Popov a.k.a. PhoeniX

#include <stdint.h>
#include <stddef.h>

#include "memop.hpp"
#include "mutex.hpp"
#include "portio.hpp"
#include "sprintf.hpp"

#define MODDESC(k, v) \
  extern const char \
    __attribute__((section(".module"),aligned(1))) \
    module_ ## k[] = v;

extern "C" {
  // Syscalls
  void puts(const char *);
  void exit(int);

  void kread(void *, const void *, size_t);

  void ioprovide(const char *, const void *);

  // Entry point
  void module();
}
