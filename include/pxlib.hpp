//    PhoeniX OS Core library functions
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
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

#define MAX(a, b) ({ \
  __typeof__(a) _a = (a); \
  __typeof__(b) _b = (b); \
  _a > _b ? _a : _b; \
})
#define MIN(a, b) ({ \
  __typeof__(a) _a = (a); \
  __typeof__(b) _b = (b); \
  _a < _b ? _a : _b; \
})
#define ABS(a)   ({ \
  __typeof__(a) _a = (a); \
  _a > 0 ? _a : -_a; \
})

class Mutex {
 private:
  bool state;
 public:
  Mutex() {
    state = 0;
  }
  void lock();
  void release();
};

extern "C" {
  extern size_t itoa(uint64_t value, char * str, uint8_t base = 10);

  void puts(const char *str);

  int printf(const char *format, ...);
  int snprintf(char *str, size_t size, const char *format, ...);

  int vprintf(const char *format, va_list ap);
  int vsnprintf(char *str, size_t size, const char *format, va_list ap);

  void clrscr();
  size_t strlen(const char*, size_t limit = -1);
  char* strdup(const char*);
  int strcmp(const char*, const char*);
  void static_init();
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
  inline static uint64_t rdtsc() {
    uint32_t eax, edx;
    asm volatile("rdtsc":"=a"(eax), "=d"(edx));
    return (((uint64_t)edx << 32) | eax);
  }
  inline static uint64_t EnterCritical() {
    uint64_t flags;
    asm volatile("pushfq; cli; pop %q0":"=r"(flags));
    return flags;
  }
  inline static void LeaveCritical(uint64_t flags) {
    asm volatile("push %q0; popfq"::"r"(flags));
  }
}
