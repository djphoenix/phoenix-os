//    PhoeniX OS Kernel library mutex functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once

#include <stdint.h>

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
  constexpr Mutex(): state(0) {}
  inline void lock() {
    asm volatile(
        "1:"
        "testw $1, %0; jnz 1b;"
        "lock btsw $0, %0; jc 1b;"
        ::"m"(state):"cc","memory");
  }
  inline bool try_lock() {
    bool ret = 0;
    asm volatile(
        "lock btsw $0, %1; jc 1f;"
        "mov $1, %0;"
        "1:"
        :"+r"(ret):"m"(state):"cc","memory");
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

class RWMutex {
 private:
  union {
    uint64_t raw;
    struct {
      uint64_t slock:1, wlock:1, rcnt:62;
    };
  } state;

  void slock() {
    asm volatile(
        "1:"
        "testw $1, %0; jnz 1b;"
        "lock btsw $0, %0; jc 1b;"
        ::"m"(state.raw):"cc","memory");
  }
  void srelease() {
    asm volatile("btcw $0, %0"::"m"(state.raw));
  }
 public:
  constexpr RWMutex(): state { .raw = 0 } {}
  inline void wlock() { while (!try_wlock()) ; }
  inline void rlock() { while (!try_rlock()) ; }
  inline bool try_wlock() {
    if (state.rcnt != 0 && state.wlock == true) return false;
    slock();
    bool res = state.rcnt == 0 && state.wlock == false;
    if (res) state.wlock = true;
    srelease();
    return res;
  }
  inline void wrelease() {
    slock();
    state.wlock = false;
    srelease();
  }
  inline bool try_rlock() {
    if (state.wlock == true) return false;
    slock();
    bool res = state.wlock == false;
    if (res) state.rcnt++;
    srelease();
    return res;
  }
  inline void rrelease() {
    slock();
    state.rcnt--;
    srelease();
  }
  class WLock {
   private:
    RWMutex *mutex;
   public:
    explicit WLock(RWMutex *mutex) : mutex(mutex) { mutex->wlock(); }
    explicit WLock(RWMutex &mutex) : WLock(&mutex) {}
    ~WLock() { mutex->wrelease(); }
  };
  class CriticalWLock: public CriticalSection {
   private:
    RWMutex *mutex;
   public:
    explicit CriticalWLock(RWMutex *mutex) : mutex(mutex) { mutex->wlock(); }
    explicit CriticalWLock(RWMutex &mutex) : CriticalWLock(&mutex) {}
    ~CriticalWLock() { mutex->wrelease(); }
  };
  class RLock {
   private:
    RWMutex *mutex;
   public:
    explicit RLock(RWMutex *mutex) : mutex(mutex) { mutex->rlock(); }
    explicit RLock(RWMutex &mutex) : RLock(&mutex) {}
    ~RLock() { mutex->rrelease(); }
  };
  class CriticalRLock: public CriticalSection {
   private:
    RWMutex *mutex;
   public:
    explicit CriticalRLock(RWMutex *mutex) : mutex(mutex) { mutex->rlock(); }
    explicit CriticalRLock(RWMutex &mutex) : CriticalRLock(&mutex) {}
    ~CriticalRLock() { mutex->rrelease(); }
  };
};
