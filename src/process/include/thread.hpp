//    PhoeniX OS Thread subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"

struct Thread {
  struct {
    uint64_t rip, rflags;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t rax, rcx, rdx, rbx;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
  } regs;
  uint64_t suspend_ticks;
  uint64_t stack_top;
  constexpr Thread() : regs(), suspend_ticks(0), stack_top(0) {}
};
