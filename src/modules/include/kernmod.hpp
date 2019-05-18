//    PhoeniX OS module includes
//    Copyright © 2019 Yury Popov a.k.a. PhoeniX

#include <stdint.h>
#include <stddef.h>

#define MODDESC(k, v) \
  extern const char \
    __attribute__((section(".module"),aligned(1))) \
    module_ ## k[] = v;

extern "C" {
  // Syscalls
  void puts(const char *);
  void exit(int);

  // Entry point
  void module();
}
