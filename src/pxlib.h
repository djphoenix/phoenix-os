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
extern char inportb(short);
extern short inports(short);
extern uint inportl(short);
extern void outportb(short, char);
extern void outports(short, short);
extern void outportl(short, uint);
#include "memory.h"
#endif
