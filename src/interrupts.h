#ifndef INTERRUPTS_H
#define INTERRUPTS_H
#include "pxlib.h"
typedef struct {
	char mov_al;
	char al;
	char push;
	long addr;
	char ret;
} __attribute__ ((packed)) HCODE, *PHCODE;
typedef struct {
	short offset_low;
	short selector;
	char zero;
	char type;
	short offset_middle;
	long offset_high;
	long reserved;
} INTERRUPT, *PINTERRUPT;
typedef struct {
	short limit;
	void* addr;
} __attribute__ ((packed)) IDTR, *PIDTR;
typedef struct {
	INTERRUPT ints[256];
	IDTR rec;
} IDT, *PIDT;
extern void interrupts_init();
#endif
