//    PhoeniX OS Core library functions
//    Copyright (C) 2013  PhoeniX
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "pxlib.hpp"
#include "memory.hpp"
char* display = (char*)0xB8000;
Mutex display_lock = Mutex();
void clrscr()
{
	display_lock.lock();
	display = (char*)0xB8FA0;
	while(display != (char*)0xB8000)
		((_uint64*)(display-=8))[0] = 0x0F000F000F000F00;
	display_lock.release();
}
void print(const char* str)
{
	display_lock.lock();
	_int64 i = 0; char c;
	while((c = str[i++])){
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
	display_lock.release();
}

static inline char HEX_CHAR(char i) {
	i &= 0xF;
	if (i > 9) return 'A' + i - 10;
	return '0' + (i & 0xF);
}

static inline void HEX_STR(_int64 val, int len, char* buf) {
	for (int i = 0; i < len; i++) {
		buf[i] = HEX_CHAR(val >> (4 * (len-i-1)));
	}
	buf[len] = 0;
}

void printb(char i)
{
	char c[3];
	HEX_STR(i,2,c);
	print(c);
}

void prints(short i)
{
	char c[5];
	HEX_STR(i,4,c);
	print(c);
}

void printl(int i)
{
	char c[9];
	HEX_STR(i,8,c);
	print(c);
}

void printq(_int64 i)
{
	char c[17];
	HEX_STR(i,16,c);
	print(c);
}

_uint64 strlen(char* c, _uint64 limit)
{
	for(_uint64 i = 0; i < limit; i++)
		if(c[i] == 0) return i;
	return 0;
}

char* strcpy(char* c)
{
	char* r = (char*)Memory::alloc(strlen(c)+1);
	for(_uint64 i = 0; i < strlen(c); i++){
		r[i] = c[i];
	}
	r[strlen(c)] = 0;
	return r;
}

bool strcmp(const char* a, char* b)
{
	int i = 0;
	while (true) {
		if (a[i] != b[i]) return false;
		if (a[i] == 0) return true;
		i++;
	}
}

extern "C" void __cxa_pure_virtual() { while (1); }

extern "C"
{
	extern void (**__init_start__)();
	extern void (**__init_end__)();
	
}

void static_init()
{
	for (void (**p)() = __init_start__; p < __init_end__; ++p)
		(*p)();
}

void Mutex::lock()
{
	bool ret_val = 0,old_val = 0,new_val = 1;
	do {
		asm volatile("lock cmpxchgb %1,%2":
					 "=a"(ret_val):
					 "r"(new_val),"m"(state),"0"(old_val):
					 "memory");
	} while (ret_val);
}
void Mutex::release()
{
	state = 0;
}