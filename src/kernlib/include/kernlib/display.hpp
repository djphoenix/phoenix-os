//    PhoeniX OS Kernel library display functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "std.hpp"
#include "mutex.hpp"

class Display {
 private:
  static Mutex instanceMutex;
  static Display *instance;
  static void setup();
 protected:
  Mutex mutex;
 public:
  static Display *getInstance();
  virtual void clean() = 0;
  virtual void write(const char*) = 0;
  virtual ~Display() CONST = 0;
};
