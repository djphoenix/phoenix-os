#include "pxlib.h"

_uint64 rdtsc(){
	unsigned long eax, edx;
	asm("rdtsc":"=a"(eax),"=d"(edx));
	_uint64 ret;
	ret = edx; ret <<= 32; ret |= eax;
	return ret;
}

int main()
{
	clrscr();
	memory_init();
	interrupts_init();
	_uint64 start, end;
	for(;;){
		print("255 allocs of 16 bytes");
		start = rdtsc();
		for(int i=0; i<255; i++) malloc(0x10);
		end = rdtsc();
		print(" in 0x");printq(end-start);print(" tacts\n");
	}
}
