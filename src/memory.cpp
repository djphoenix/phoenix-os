#include "pxlib.h"
PGRUB grub_data;
void l2h(const _int64 i, char* s){
	for(int c = 0; c < sizeof(i)/2; c++){
		char b = (i >> ((sizeof(i)-c-1)*8)) & 0xFF;
		s[c*2+1] = ((b & 0xF) > 9) ? ('A' + (b & 0xF) - 10) : ('0' + (b & 0xF));
		b >>= 4;
		s[c*2  ] = ((b & 0xF) > 9) ? ('A' + (b & 0xF) - 10) : ('0' + (b & 0xF));
	}
	for(int c = sizeof(i)/2; c < sizeof(i); c++){
		char b = (i >> ((sizeof(i)-c-1)*8)) & 0xFF;
		s[c*2+2] = ((b & 0xF) > 9) ? ('A' + (b & 0xF) - 10) : ('0' + (b & 0xF));
		b >>= 4;
		s[c*2+1] = ((b & 0xF) > 9) ? ('A' + (b & 0xF) - 10) : ('0' + (b & 0xF));
	}
}
void memory_init()
{
	print("Initializing memory...\n");
	_int64 kernel = 0, stack = 0, data = 0, data_top = 0, bss = 0, bss_top = 0;
	asm("movq $_start, %q0":"=a"(kernel));
	asm("movq $__data_start__, %q0":"=a"(data));
	asm("movq $__data_end__, %q0":"=a"(data_top)); data_top = (data_top & 0xFFFFF000) + 0x1000;
	asm("movq $__bss_start__, %q0":"=a"(bss)); bss_top = (bss_top & 0xFFFFF000) + 0x1000;
	asm("movq $__bss_end__, %q0":"=a"(bss_top)); bss_top = (bss_top & 0xFFFFF000) + 0x1000;

	char*
		l= "GRUB loader data at		0x00000000 00000000\n";
	l2h((_int64) grub_data,&(l[23])); print(l);

		l= "Kernel code start at	0x00000000 00000000\n";
	l2h(kernel,&(l[23])); print(l);

		l= "Data section start at	0x00000000 00000000\n";
	l2h(data,&(l[24])); print(l);

		l= "Data section end at		0x00000000 00000000\n";
	l2h(data_top,&(l[23])); print(l);

		l= "BSS section start at	0x00000000 00000000\n";
	l2h(bss,&(l[23])); print(l);

		l= "BSS section end at		0x00000000 00000000\n";
	l2h(bss_top,&(l[22])); print(l);
}
