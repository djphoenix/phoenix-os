//    PhoeniX OS Kernel library standard functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once

#include <stdint.h>

class RAND {
 private:
  static uint64_t _get64();
 public:
  template<typename T> static inline T get() { return static_cast<T>(_get64()); }
};
