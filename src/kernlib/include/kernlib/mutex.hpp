//    PhoeniX OS Kernel library mutex functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "std.hpp"

class CriticalSection {
 private:
  uint64_t flags;

 public:
  inline static uint64_t __attribute__((always_inline)) enter() {
    uint64_t flags;
    asm volatile("pushfq; cli; pop %q0":"=r"(flags));
    return flags;
  }

  inline static void leave(uint64_t flags) {
    asm volatile("push %q0; popfq"::"r"(flags));
  }

  CriticalSection() : flags(enter()) {}
  ~CriticalSection() { leave(flags); }
};

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
  class Lock {
   private:
    Mutex *mutex;
   public:
    explicit Lock(Mutex *mutex) : mutex(mutex) { mutex->lock(); }
    explicit Lock(Mutex &mutex) : Lock(&mutex) {}
    ~Lock() { mutex->release(); }
  };
  class CriticalLock: public CriticalSection {
   private:
    Mutex *mutex;
   public:
    explicit CriticalLock(Mutex *mutex) : mutex(mutex) { mutex->lock(); }
    explicit CriticalLock(Mutex &mutex) : CriticalLock(&mutex) {}
    ~CriticalLock() { mutex->release(); }
  };
};
