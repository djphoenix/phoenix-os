#include "pxlib.h"
char* display = (char*)0xB8000;
void clrscr()
{
	display = (char*)0xB8FA0;
	while(display != (char*)0xB8000)
		((_uint64*)(display-=8))[0] = 0x0F000F000F000F00;
}
void print(const char* str)
{
	_int64 i = 0; char c;
	while(c = str[i++]){
		if(c == 0) return;
		if(c == 10) display += 160 - (((_uint64)display - 0xB8000) % 160);
		else if(c == 9) {
			do{
				display[0] = ' ';
				display+=2;
			} while((_uint64)display % 8 != 0);
		} else {
			display[0] = c;
			display+=2;
		}
		if(display >= (char*)0xB8FA0) {
			display = (char*)0xB8000;
			while(display != (char*)0xB8F00) {
			((_uint64*)(display))[0] = ((_uint64*)(display+160))[0];
			display += 8;
			}
			display = (char*)0xB8FA0;
			while(display != (char*)0xB8F00) ((_uint64*)(display-=8))[0] = 0x0F000F000F000F00;
		}
	}
}

void printb(char i)
{
	char c[3];
	c[1] = (i & 0xF) > 9 ? 'A' + (i & 0xF) - 10 : '0' + (i & 0xF);
	i = i >> 4;
	c[0] = (i & 0xF) > 9 ? 'A' + (i & 0xF) - 10 : '0' + (i & 0xF);
	c[2] = 0;
	print(c);
}

void prints(short i)
{
	printb((i >> 8) & 0xFF);
	printb(i & 0xFF);
}

void printl(int i)
{
	prints((i >> 16) & 0xFFFF);
	prints(i & 0xFFFF);
}

void printq(_int64 i)
{
	printl((i >> 32) & 0xFFFFFFFF);
	printl(i & 0xFFFFFFFF);
}

char __inline inportb(short port){ char c; asm("inb %w1, %b0":"=a"(c):"d"(port)); return c; }
short __inline inports(short port){ short c; asm("inw %w1, %w0":"=a"(c):"d"(port)); return c; }
uint __inline inportl(short port){ uint c; asm("inl %w1, %d0":"=a"(c):"d"(port)); return c; }
void __inline outportb(short port, char c){ asm("outb %b0, %b1"::"a"(c),"d"(port)); }
void __inline outports(short port, short c){ asm("outw %w0, %w1"::"a"(c),"d"(port)); }
void __inline outportl(short port, uint c){ asm("outl %d0, %d1"::"a"(c),"d"(port)); }
