#include "pxlib.h"

int main()
{
	clrscr();
	memory_init();
	malloc(0x3800);
	malloc(0x800);
	memmap();
}
