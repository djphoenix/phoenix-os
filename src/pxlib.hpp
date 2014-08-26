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

#ifndef PXLIB_H
#define PXLIB_H

#define MAX(A,B) ((A)>(B))?(A):(B)

typedef unsigned int uint;
typedef long long int _int64;
typedef unsigned long long int _uint64;
extern void print(const char*);
extern void printb(char i);
extern void prints(short i);
extern void printl(int i);
extern void printq(_int64 i);
extern void clrscr();
extern _uint64 strlen(char*);
extern char* strcpy(char*);
extern bool strcmp(const char*,char*);
char __inline inportb(short port){ char c; asm("inb %w1, %b0":"=a"(c):"d"(port)); return c; }
short __inline inports(short port){ short c; asm("inw %w1, %w0":"=a"(c):"d"(port)); return c; }
uint __inline inportl(short port){ uint c; asm("inl %w1, %d0":"=a"(c):"d"(port)); return c; }
void __inline outportb(short port, char c){ asm("outb %b0, %w1"::"a"(c),"d"(port)); }
void __inline outports(short port, short c){ asm("outw %w0, %w1"::"a"(c),"d"(port)); }
void __inline outportl(short port, uint c){ asm("outl %d0, %w1"::"a"(c),"d"(port)); }
_uint64 __inline rdtsc() { unsigned long eax, edx; asm("rdtsc":"=a"(eax),"=d"(edx)); _uint64 ret; ret = edx; ret <<= 32; ret |= eax; return ret; }
#endif
