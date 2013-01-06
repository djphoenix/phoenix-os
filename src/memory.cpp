#include "pxlib.h"
PGRUB grub_data;
void l2h(const _int64 i, char* s){
	for(int c = 0; c < sizeof(i); c++){
		char b = (i >> ((sizeof(i)-c-1)*8)) & 0xFF;
		s[c*2+1] = ((b & 0xF) > 9) ? ('A' + (b & 0xF) - 10) : ('0' + (b & 0xF));
		b >>= 4;
		s[c*2  ] = ((b & 0xF) > 9) ? ('A' + (b & 0xF) - 10) : ('0' + (b & 0xF));
	}
}
void memory_init()
{
	print("Initializing memory...\n");
	_int64 kernel = 0, stack = 0, kernel_top = 0;
	kernel = (_int64) grub_data;
	char* l = "GRUB bios data at '0x0000000000000000'\n";
	l2h(kernel,&(l[21]));
	print(l);
	l = "Kernel loaded into '0x0000000000000000'\n";
	asm("movq $_start, %q0":"=a"(kernel));
	l2h(kernel,&(l[22]));
	print(l);
	asm("movq $_data_start, %q0":"=a"(kernel));
	l = "Data section loaded into '0x0000000000000000'\n";
	l2h(kernel,&(l[28]));
	print(l);
}
