#include "pxlib.h"

int main()
{
	clrscr();
	memory_init();
	interrupts_init();
	for(int i=0; i<5000; i++) {
		print("Alloc: "); prints(i); print("\n");
		malloc(100);
	}
}
