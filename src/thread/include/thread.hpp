//    PhoeniX OS Thread subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once

#include <stdint.h>

struct Thread {
  uint64_t rip, rflags, rsp;
  struct Regs {
    uint64_t rax, rcx, rdx, rbx;
    uint64_t rbp, rsi, rdi;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
  } __attribute__((packed)) regs;
  struct SSE {
    uint64_t sse[64];
  } __attribute__((packed)) sse;
  uint64_t suspend_ticks;
  uint64_t stack_top;
  constexpr Thread() : regs(), suspend_ticks(0), stack_top(0) {}
};
