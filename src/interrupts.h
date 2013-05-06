#ifndef INTERRUPTS_H
#define INTERRUPTS_H
#include "pxlib.h"
typedef struct {
	short push_w;
	short intr;
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
} INTERRUPT32, *PINTERRUPT32;
typedef struct {
	short offset_low;
	short selector;
	char zero;
	char type;
	short offset_middle;
	long offset_high;
	long reserved;
} INTERRUPT64, *PINTERRUPT64;
typedef struct {
	short limit;
	void* addr;
} __attribute__ ((packed)) IDTR, *PIDTR;
typedef struct {
	INTERRUPT64 ints[256];
	IDTR rec;
} IDT, *PIDT;
extern INTERRUPT32 interrupts32[256];
extern void interrupts_init();
#endif
