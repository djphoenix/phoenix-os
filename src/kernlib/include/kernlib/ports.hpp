//    PhoeniX OS Kernel library port I/O functions
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
