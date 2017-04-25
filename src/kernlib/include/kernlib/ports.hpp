//    PhoeniX OS Kernel library port I/O functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "std.hpp"

inline static uint8_t inportb(uint16_t port) {
  uint8_t c;
  asm volatile("inb %w1, %b0":"=a"(c):"d"(port));
  return c;
}

inline static uint16_t inports(uint16_t port) {
  uint16_t c;
  asm volatile("inw %w1, %w0":"=a"(c):"d"(port));
  return c;
}

inline static uint32_t inportl(uint16_t port) {
  uint32_t c;
  asm volatile("inl %w1, %d0":"=a"(c):"d"(port));
  return c;
}

inline static void outportb(uint16_t port, uint8_t c) {
  asm volatile("outb %b0, %w1"::"a"(c), "d"(port));
}

inline static void outports(uint16_t port, uint16_t c) {
  asm volatile("outw %w0, %w1"::"a"(c), "d"(port));
}

inline static void outportl(uint16_t port, uint32_t c) {
  asm volatile("outl %d0, %w1"::"a"(c), "d"(port));
}
