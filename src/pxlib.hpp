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

#define MAX(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#define MIN(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
#define ABS(a)   ({ __typeof__ (a) _a = (a); _a > 0 ? _a : -_a; })

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

typedef uint64_t _uint64;
typedef uint64_t size_t;

extern "C" {
	extern size_t itoa(uint64_t value, char * str, uint8_t base = 10);
	extern size_t printf(const char *fmt, ...);
	extern size_t vprintf(const char *fmt, va_list args);
	
	extern void clrscr();
	extern size_t strlen(const char*,size_t limit=-1);
	extern char* strcpy(const char*);
	extern bool strcmp(const char*,char*);
	extern void static_init();
	uint8_t __inline inportb(uint16_t port){ uint8_t c; asm("inb %w1, %b0":"=a"(c):"d"(port)); return c; }
	uint16_t __inline inports(uint16_t port){ uint16_t c; asm("inw %w1, %w0":"=a"(c):"d"(port)); return c; }
	uint32_t __inline inportl(uint16_t port){ uint32_t c; asm("inl %w1, %d0":"=a"(c):"d"(port)); return c; }
	void __inline outportb(uint16_t port, uint8_t c){ asm("outb %b0, %w1"::"a"(c),"d"(port)); }
	void __inline outports(uint16_t port, uint16_t c){ asm("outw %w0, %w1"::"a"(c),"d"(port)); }
	void __inline outportl(uint16_t port, uint32_t c){ asm("outl %d0, %w1"::"a"(c),"d"(port)); }
	uint64_t __inline rdtsc() { uint32_t eax, edx; asm("rdtsc":"=a"(eax),"=d"(edx)); uint64_t ret; ret = edx; ret <<= 32; ret |= eax; return ret; }
}
