//    PhoeniX OS Thread subsystem
//    Copyright (C) 2013  PhoeniX
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once
#include "pxlib.hpp"

class Thread {
 public:
  Thread();
  struct {
    uint64_t rip, rflags;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t rax, rcx, rdx, rbx;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
  } regs;
  uint64_t suspend_ticks;
  uint64_t stack_top;
};
