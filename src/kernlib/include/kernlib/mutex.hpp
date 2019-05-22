//    PhoeniX OS Kernel library mutex functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "std.hpp"

class Mutex {
 private:
  bool state;
 public:
  Mutex(): state(0) {}
  inline void lock() {
    asm volatile(
        "1:"
        "testw $1, %0; jnz 1b;"
        "lock btsw $0, %0; jc 1b;"
        ::"m"(state));
  }
  inline bool try_lock() {
    bool ret = 0;
    asm volatile(
        "lock btsw $0, %1; jc 1f;"
        "mov $1, %0;"
        "1:"
        :"=r"(ret):"m"(state));
    return ret;
  }
  inline void release() {
    asm volatile("movw $0, %0"::"m"(state));
  }
};
