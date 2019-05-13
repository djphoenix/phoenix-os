//    PhoeniX OS Kernel library port I/O functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "std.hpp"

template<uint16_t port> struct Port {
  template<typename T> inline static T in();
  template<typename T> inline static void out(T value);

  template<> inline static uint8_t in() {
    uint8_t c;
    asm volatile("inb %w1, %b0":"=a"(c):"d"(port));
    return c;
  }

  template<> inline static uint16_t in() {
    uint16_t c;
    asm volatile("inw %w1, %w0":"=a"(c):"d"(port));
    return c;
  }

  template<> inline static uint32_t in() {
    uint32_t c;
    asm volatile("inl %w1, %d0":"=a"(c):"d"(port));
    return c;
  }

  template<> inline static void out(uint8_t c) {
    asm volatile("outb %b0, %w1"::"a"(c), "d"(port));
  }

  template<> inline static void out(uint16_t c) {
    asm volatile("outw %w0, %w1"::"a"(c), "d"(port));
  }

  template<> inline static void out(uint32_t c) {
    asm volatile("outl %d0, %w1"::"a"(c), "d"(port));
  }
};
