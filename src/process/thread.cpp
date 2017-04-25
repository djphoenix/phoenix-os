//    PhoeniX OS Thread subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "thread.hpp"

Thread::Thread() {
  regs = {
    0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
  };
  suspend_ticks = 0;
  stack_top = 0;
}
