//    PhoeniX OS System calls subsystem
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

#include "syscall.hpp"

static const uint32_t MSR_EFER = 0xC0000080;
static const uint32_t MSR_STAR = 0xC0000081;
static const uint32_t MSR_LSTAR = 0xC0000082;
static const uint64_t MSR_EFER_SCE = 1 << 0;
static const uint16_t KERNEL_CS = 8;
static const uint16_t USER_CS = 24;

static inline void wrmsr(uint32_t msr_id, uint64_t msr_value) {
  asm volatile("wrmsr"::"c"(msr_id), "A"(msr_value), "d"(msr_value >> 32));
}

static inline uint64_t rdmsr(uint32_t msr_id) {
  uint64_t msr_hi, msr_lo;
  asm volatile("rdmsr":"=A"(msr_lo), "=d"(msr_hi):"c"(msr_id));
  return (msr_hi << 32) | msr_lo;
}

void __attribute((naked)) Syscall::wrapper() {
  asm volatile(
      "sysretq;"
  );
}

void Syscall::setup() {
  wrmsr(MSR_STAR,
      ((uint64_t)USER_CS) << 48 |
      ((uint64_t)KERNEL_CS) << 32);
  wrmsr(MSR_LSTAR, (uintptr_t)wrapper);
  wrmsr(MSR_EFER, rdmsr(MSR_EFER) | MSR_EFER_SCE);
}
