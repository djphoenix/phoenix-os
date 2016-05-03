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
bool display_lock = 0;
void clrscr()
{
	display = (char*)0xB8FA0;
	while(display != (char*)0xB8000)
		((_uint64*)(display-=8))[0] = 0x0F000F000F000F00;
}
void print(const char* str)
{
    while (display_lock);
    display_lock = 1;
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
    display_lock=0;
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