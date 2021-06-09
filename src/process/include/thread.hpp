//    PhoeniX OS Thread subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"
#include "interrupts.hpp"

struct Thread {
  uint64_t rip, rflags, rsp;
  Interrupts::CallbackRegs::General regs;
  Interrupts::CallbackRegs::SSE sse;
  uint64_t suspend_ticks;
  uint64_t stack_top;
  constexpr Thread() : regs(), suspend_ticks(0), stack_top(0) {}
};
