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
void clrscr() {
	display_lock.lock();
	display = (char*)0xB8FA0;
	while(display != (char*)0xB8000)
		((_uint64*)(display -= 8))[0] = 0x0F000F000F000F00;
	display_lock.release();
}
void putc(const char c) {
	if(c == 0) return;
	if(c == 10) {
		display += 160 - (((_uint64)display - 0xB8000) % 160);
	} else if (c == 9) {
		do {
			display[0] = ' ';
			display+=2;
		} while ((_uint64)display % 8 != 0);
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
		while(display != (char*)0xB8F00)
			((_uint64*)(display -= 8))[0] = 0x0F000F000F000F00;
	}
}

size_t itoa(uint64_t value, char * str, uint8_t base) {
	char * ptr;
	char * low;
	int len = 0;
	if ( base < 2 || base > 36 ) {
		*str = '\0';
		return len;
	}
	ptr = str;
	if ( value < 0 && base == 10 ) {
		*ptr++ = '-';
	}
	low = ptr;
	do {
		*ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[ABS(value) % base];
		value /= base;
		len++;
	} while ( value );
	*ptr-- = '\0';
	while ( low < ptr ) {
		char tmp = *low;
		*low++ = *ptr;
		*ptr-- = tmp;
	}
	return len;
}

#define LONGFLAG     0x00000001
#define LONGLONGFLAG 0x00000002
#define HALFFLAG     0x00000004
#define HALFHALFFLAG 0x00000008
#define SIZETFLAG    0x00000010
#define ALTFLAG      0x00000020
#define CAPSFLAG     0x00000040
#define SHOWSIGNFLAG 0x00000080
#define SIGNEDFLAG   0x00000100
#define LEFTFORMATFLAG 0x00000200
#define LEADZEROFLAG 0x00000400

static char *longlong_to_string(char *buf,
								uint64_t n,
								size_t len,
								uint32_t flag) {
	int pos = len;
	int negative = 0;
	if ((flag & SIGNEDFLAG) && (int64_t)n < 0) {
		negative = 1;
		n = -n;
	}
	buf[--pos] = 0;
	while(n >= 10) {
		int digit = n % 10;
		n /= 10;
		buf[--pos] = digit + '0';
	}
	buf[--pos] = n + '0';
	if(negative) buf[--pos] = '-';
	else if ((flag & SHOWSIGNFLAG)) buf[--pos] = '+';
	return &buf[pos];
}
static char *longlong_to_hexstring(char *buf,
								   uint64_t u,
								   size_t len,
								   uint32_t flag) {
	int pos = len;
	static const char hextable[] = "0123456789abcdef";
	static const char hextable_caps[] = "0123456789ABCDEF";
	const char *table = (flag & CAPSFLAG) ? hextable_caps : hextable;
	buf[--pos] = 0;
	do {
		unsigned int digit = u % 16;
		u /= 16;
		buf[--pos] = table[digit];
	} while (u != 0);
	return &buf[pos];
}

size_t vprintf(const char *fmt, va_list ap) {
	display_lock.lock();
	char c;
	unsigned char uc;
	const char *s;
	uint64_t n;
	void *ptr;
	int flags;
	unsigned int format_num;
	size_t chars_written = 0;
	char num_buffer[32];
#define OUTPUT_CHAR(c) do {\
	char _c = c; \
	if (_c == 0) goto done; \
	putc(_c); \
} while (0)
	for(;;) {
		while((c = *fmt++) != 0) {
			if(c == '%')
				break;
			OUTPUT_CHAR(c);
		}
		if(c == 0)
			break;
		flags = 0;
		format_num = 0;
	next_format:
		c = *fmt++;
		if(c == 0)
			break;
		
		switch(c) {
			case '0'...'9':
				if (c == '0' && format_num == 0)
					flags |= LEADZEROFLAG;
				format_num *= 10;
				format_num += c - '0';
				goto next_format;
			case '.':
				goto next_format;
			case '%':
				OUTPUT_CHAR('%');
				break;
			case 'c':
				uc = va_arg(ap, unsigned int);
				OUTPUT_CHAR(uc);
				break;
			case 's':
				s = va_arg(ap, const char *);
				if(s == 0)
					s = "<null>";
				goto _output_string;
			case '-':
				flags |= LEFTFORMATFLAG;
				goto next_format;
			case '+':
				flags |= SHOWSIGNFLAG;
				goto next_format;
			case '#':
				flags |= ALTFLAG;
				goto next_format;
			case 'l':
				if(flags & LONGFLAG)
					flags |= LONGLONGFLAG;
				flags |= LONGFLAG;
				goto next_format;
			case 'h':
				if(flags & HALFFLAG)
					flags |= HALFHALFFLAG;
				flags |= HALFFLAG;
				goto next_format;
			case 'z':
				flags |= SIZETFLAG;
				goto next_format;
			case 'D':
				flags |= LONGFLAG;
				/* fallthrough */
			case 'i':
			case 'd':
				n = (flags & LONGLONGFLAG) ? va_arg(ap, int64_t) :
				(flags & LONGFLAG) ? va_arg(ap, int32_t) :
				(flags & HALFHALFFLAG) ? (int8_t)va_arg(ap, int32_t) :
				(flags & HALFFLAG) ? (int16_t)va_arg(ap, int32_t) :
				(flags & SIZETFLAG) ? va_arg(ap, size_t) :
				va_arg(ap, int32_t);
				flags |= SIGNEDFLAG;
				s = longlong_to_string(num_buffer, n, sizeof(num_buffer), flags);
				goto _output_string;
			case 'U':
				flags |= LONGFLAG;
				/* fallthrough */
			case 'u':
				n = (flags & LONGLONGFLAG) ? va_arg(ap, uint64_t) :
				(flags & LONGFLAG) ? va_arg(ap, uint32_t) :
				(flags & HALFHALFFLAG) ? (uint8_t)va_arg(ap, uint32_t) :
				(flags & HALFFLAG) ? (uint16_t)va_arg(ap, uint32_t) :
				(flags & SIZETFLAG) ? va_arg(ap, size_t) :
				va_arg(ap, uint32_t);
				s = longlong_to_string(num_buffer, n, sizeof(num_buffer), flags);
				goto _output_string;
			case 'p':
				flags |= LONGFLAG | ALTFLAG;
				goto hex;
			case 'X':
				flags |= CAPSFLAG;
				/* fallthrough */
			hex:
			case 'x':
				n = (flags & LONGLONGFLAG) ? va_arg(ap, uint64_t) :
				(flags & LONGFLAG) ? va_arg(ap, uint32_t) :
				(flags & HALFHALFFLAG) ? (uint8_t)va_arg(ap, uint32_t) :
				(flags & HALFFLAG) ? (uint16_t)va_arg(ap, uint32_t) :
				(flags & SIZETFLAG) ? va_arg(ap, size_t) :
				va_arg(ap, uint32_t);
				s = longlong_to_hexstring(num_buffer, n, sizeof(num_buffer), flags);
				if(flags & ALTFLAG) {
					OUTPUT_CHAR('0');
					OUTPUT_CHAR((flags & CAPSFLAG) ? 'X': 'x');
				}
				goto _output_string;
			case 'n':
				ptr = va_arg(ap, void *);
				if(flags & LONGLONGFLAG)
					*(int64_t *)ptr = chars_written;
				else if (flags & LONGFLAG)
					*(int32_t *)ptr = chars_written;
				else if (flags & HALFHALFFLAG)
					*(int8_t *)ptr = chars_written;
				else if (flags & HALFFLAG)
					*(int16_t *)ptr = chars_written;
				else if (flags & SIZETFLAG)
					*(size_t *)ptr = chars_written;
				else
					*(int32_t *)ptr = chars_written;
				break;
			default:
				OUTPUT_CHAR('%');
				OUTPUT_CHAR(c);
				break;
		}
		continue;
	_output_string:
		if (flags & LEFTFORMATFLAG) {
			uint32_t count = 0;
			while(*s != 0) {
				OUTPUT_CHAR(*s++);
				count++;
			}
			for (; format_num > count; format_num--)
				OUTPUT_CHAR(' ');
		} else {
			_uint64 string_len = strlen(s);
			char outchar = (flags & LEADZEROFLAG) ? '0' : ' ';
			for (; format_num > string_len; format_num--)
				OUTPUT_CHAR(outchar);
			while(*s != 0)
				OUTPUT_CHAR(*s++);
		}
		continue;
	}
done:
#undef OUTPUT_CHAR
#undef OUTPUT_CHAR_NOLENCHECK
	display_lock.release();
	return chars_written;
}

size_t printf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int len = vprintf(fmt, args);
	va_end(args);
	return len;
}

size_t strlen(const char* c, size_t limit) {
	for(size_t i = 0; i < limit; i++)
		if(c[i] == 0) return i;
	return 0;
}

char* strcpy(const char* c) {
	char* r = (char*)Memory::alloc(strlen(c)+1);
	for(uint64_t i = 0; i < strlen(c); i++) {
		r[i] = c[i];
	}
	r[strlen(c)] = 0;
	return r;
}

bool strcmp(const char* a, char* b) {
	int i = 0;
	while (true) {
		if (a[i] != b[i]) return false;
		if (a[i] == 0) return true;
		i++;
	}
}

extern "C" {
	void __cxa_pure_virtual() { while (1); }
	extern void (*__init_start__)();
	extern void (*__init_end__)();
}

void static_init() {
	for (void (**p)() = &__init_start__; p < &__init_end__; ++p)
		(*p)();
}

void Mutex::lock() {
	bool ret_val = 0, old_val = 0, new_val = 1;
	do {
		asm volatile("lock cmpxchgb %1,%2":
					 "=a"(ret_val):
					 "r"(new_val),"m"(state),"0"(old_val):
					 "memory");
	} while (ret_val);
}
void Mutex::release() {
	state = 0;
}
