//    PhoeniX OS Kernel library mutex functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "std.hpp"

class Mutex {
 private:
  bool state;
 public:
  Mutex(): state(0) {}
  void lock();
  void release();
};
