#ifndef PXLIB_H
#define PXLIB_H
typedef unsigned int uint;
typedef long long int _int64;
typedef unsigned long long int _uint64;
extern void print(const char*);
extern void printb(char i);
extern void prints(short i);
extern void printl(int i);
extern void printq(_int64 i);
extern void clrscr();
char __inline inportb(short port){ char c; asm("inb %w1, %b0":"=a"(c):"d"(port)); return c; }
short __inline inports(short port){ short c; asm("inw %w1, %w0":"=a"(c):"d"(port)); return c; }
uint __inline inportl(short port){ uint c; asm("inl %w1, %d0":"=a"(c):"d"(port)); return c; }
void __inline outportb(short port, char c){ asm("outb %b0, %w1"::"a"(c),"d"(port)); }
void __inline outports(short port, short c){ asm("outw %w0, %w1"::"a"(c),"d"(port)); }
void __inline outportl(short port, uint c){ asm("outl %d0, %w1"::"a"(c),"d"(port)); }
_uint64 __inline rdtsc() { unsigned long eax, edx; asm("rdtsc":"=a"(eax),"=d"(edx)); _uint64 ret; ret = edx; ret <<= 32; ret |= eax; return ret; }
#include "memory.h"
#include "interrupts.h"
#include "multiboot_info.h"
#endif
