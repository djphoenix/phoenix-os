//    PhoeniX OS System calls subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once

#include <stdint.h>

class Syscall {
 public:
  static void setup();
  static uint64_t callByName(const char *name) __attribute__((pure));
 private:
  static void wrapper();
  friend class KernelLinker;
};
