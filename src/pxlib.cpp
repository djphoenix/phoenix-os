#include "pxlib.h"
char* display = (char*)0xB8000;
void clrscr(){
  display = (char*)0xB8FA0;
  while(display != (char*)0xB8000) ((_uint64*)(display-=8))[0] = 0x0F000F000F000F00;
}
void print(const char* str){
  _int64 i = 0; char c;
  while(c = str[i++]){
	if(c == 0) return;
	if(c == 10) display += 160 - (((_uint64)display - 0xB8000) % 160);
	else {
	  display[0] = c;
	  display+=2;
	}
	if(display >= (char*)0xB8FA0){
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

char inportb(short port){
  char c; asm("inb %w1, %b0":"=a"(c):"d"(port)); return c;
}
short inports(short port){
  short c; asm("inw %w1, %w0":"=a"(c):"d"(port)); return c;
}
uint inportl(short port){
  uint c; asm("inl %w1, %d0":"=a"(c):"d"(port)); return c;
}
void outportb(short port, char c){
  asm("outb %b0, %w1"::"a"(c),"d"(port));
}
void outports(short port, short c){
  asm("outw %w0, %w1"::"a"(c),"d"(port));
}
void outportl(short port, uint c){
  asm("outl %d0, %w1"::"a"(c),"d"(port));
}
