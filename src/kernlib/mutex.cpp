//    PhoeniX OS Kernel library mutex functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "kernlib.hpp"

void Mutex::lock() {
  asm volatile(
      "1:"
      "mov $1, %%dl;"
      "xor %%al, %%al;"
      "lock cmpxchgb %%dl, %0;"
      "jnz 1b"
      ::"m"(state)
       :"rax","rdx");
}

void Mutex::release() {
  asm volatile(
      "xor %%al, %%al;"
      "lock xchgb %%al, %0"
      ::"m"(state)
       :"rax");
}
