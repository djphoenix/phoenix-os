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

static char *const display_start = (char*)0xB8000;
static char *const display_top = (char*)0xB8FA0;
static const size_t display_size = display_top - display_start;

char* display = display_start;
Mutex display_lock = Mutex();

static inline void display_fill(char *base, size_t len) {
  asm volatile(
      "mov %0, %%rdi;"
      "cld;"
      "rep stosq;"
      ::
      "r"(base),
      "a"(0x0F000F000F000F00),
      "c"(len / sizeof(uint64_t))
      :"rdi"
  );
}

void clrscr() {
  display_lock.lock();
  display_fill(display_start, display_size);
  display = display_start;
  display_lock.release();
}

void putc(const char c) {
  if (c == 0)
    return;
  size_t pos = display - display_start;
  if (c == '\n') {
    display += 160 - (pos % 160);
  } else if (c == '\t') {
    do {
      display[0] = ' ';
      display += 2;
    } while (pos % 8 != 0);
  } else {
    display[0] = c;
    display += 2;
  }
  if (display >= display_top) {
    Memory::copy(display_start, display_start + 160, display_size - 160);
    display = display_top - 160;
    display_fill(display, 160);
  }
}

void puts(const char *str) {
  uint64_t t = EnterCritical();
  display_lock.lock();
  while (*str != 0)
    putc(*(str++));
  display_lock.release();
  LeaveCritical(t);
}

static char *longlong_to_string(char *buf, size_t len, uint64_t n, uint8_t base,
                                bool fl_signed, bool fl_showsign,
                                bool fl_caps) {
  int pos = len;
  char table_start = fl_caps ? 'A' : 'a';
  bool negative = 0;
  if (fl_signed && (int64_t)n < 0) {
    negative = 1;
    n = -n;
  }
  buf[ --pos] = 0;
  while (n) {
    uint8_t digit = n % base;
    n /= base;
    if (digit < 10) {
      buf[ --pos] = digit + '0';
    } else {
      buf[ --pos] = digit - 10 + table_start;
    }
  }
  if (negative) buf[ --pos] = '-';
  else if (fl_showsign) buf[ --pos] = '+';
  return &buf[pos];
}

size_t itoa(uint64_t value, char * str, uint8_t base) {
  char buf[64];
  char *ret = longlong_to_string(buf, sizeof(buf), value, base, 1, 0, 0);
  int len = strlen(ret);
  if (str) {
    Memory::copy(str, ret, len + 1);
  }
  return len;
}

static inline void printf_putc(char *str, size_t *size, int *len, char c) {
  if ((*len) == -1) return;
  (*len)++;
  if (str == 0) return;
  if (*size == 0) {
    *len = -1;
    return;
  }
  (*size)--;
  str[*len-1] = c;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
  typedef union {
    struct {
      struct {
        uint8_t fl_leftfmt :1;    // %-
        uint8_t fl_showsign :1;   // %+
        uint8_t fl_leadspace :1;  // % [space]
        uint8_t fl_altfmt :1;     // %#
        uint8_t fl_leadzero :1;   // %0
      } __attribute__((packed));
      struct {
        uint8_t sz_halfhalf :1;   // %hh
        uint8_t sz_half :1;       // %h
        uint8_t sz_long :1;       // %l
        uint8_t sz_longlong :1;   // %ll
        uint8_t sz_max :1;        // %j
        uint8_t sz_sizet :1;      // %z
        uint8_t sz_ptrdiff :1;    // %t
        uint8_t sz_longdbl :1;    // %L
      };
    } __attribute__((packed));
    struct {
      uint32_t raw_flags :5;
      uint32_t raw_size :8;
    } __attribute__((packed));
    uint32_t raw;
  } __attribute__((packed)) flags_t;

  char c;
  const char *fmt_start;
  int out_len = 0;

  flags_t flags;
  int width, prec;
  uintmax_t numval;
  char numbuf[64];
  const char *strval;

  uint8_t numbase;
  bool upcaseout, numsig;

  for (;;) {
    while ((c = *format++) != 0) {
      if (c == '%') {
        fmt_start = format;
        break;
      }
      printf_putc(str, &size, &out_len, c);
    }
    if (c == 0) break;

    flags.raw = 0;
    width = 0; prec = 0;
    numbase = 10;
    upcaseout = 0;

parse_flags:
    switch (c = *format++) {  // Flags
      case '-':
        flags.fl_leftfmt = 1;
        goto parse_flags;
      case '+':
        flags.fl_showsign = 1;
        goto parse_flags;
      case ' ':
        flags.fl_leadspace = 1;
        goto parse_flags;
      case '#':
        flags.fl_altfmt = 1;
        goto parse_flags;
      case '0':
        flags.fl_leadzero = 1;
        goto parse_flags;
      case 0: goto invalid_fmt;
      default:
        format--;
        break;
    }

parse_width:
    switch (c = *format++) {  // Width
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        width *= 10;
        width += c - '0';
        goto parse_width;
      case '*':
        if (width != 0) goto invalid_fmt;
        width = va_arg(ap, int);
        break;
      default:
        format--;
        break;
    }

    if (*format == '.') {
      format++;
parse_prec:
      switch (c = *format++) {  // Prec
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
          prec *= 10;
          prec += c - '0';
          goto parse_prec;
        case '*':
          if (prec != 0) goto invalid_fmt;
          prec = va_arg(ap, int);
          break;
        default:
          format--;
          break;
      }
    }

parse_size:
    switch (c = *format++) {  // Size
      case 'h':
        if (flags.sz_half) {
          flags.sz_half = 0;
          if (flags.raw_size != 0) goto invalid_fmt;
          flags.sz_halfhalf = 1;
        } else {
          if (flags.raw_size != 0) goto invalid_fmt;
          flags.sz_half = 1;
        }
        goto parse_size;
      case 'l':
        if (flags.sz_long) {
          flags.sz_long = 0;
          if (flags.raw_size != 0) goto invalid_fmt;
          flags.sz_longlong = 1;
        } else {
          if (flags.raw_size != 0) goto invalid_fmt;
          flags.sz_long = 1;
        }
        goto parse_size;
      case 'j':
        if (flags.raw_size != 0) goto invalid_fmt;
        flags.sz_max = 1;
        goto parse_size;
      case 'z':
        if (flags.raw_size != 0) goto invalid_fmt;
        flags.sz_sizet = 1;
        goto parse_size;
      case 't':
        if (flags.raw_size != 0) goto invalid_fmt;
        flags.sz_ptrdiff = 1;
        goto parse_size;
      case 'L':
        if (flags.raw_size != 0) goto invalid_fmt;
        flags.sz_longdbl = 1;
        goto parse_size;
      default:
        format--;
        break;
    }

    switch (c = *format++) {
      case '%':
        if (flags.raw != 0) goto invalid_fmt;
        printf_putc(str, &size, &out_len, '%');
        goto next_format;
      case 'c':
        c = va_arg(ap, /* char */ int) & 0xFF;
        numbuf[0] = c;
        numbuf[1] = 0;
        strval = numbuf;
        goto out_str;
      case 's':
        strval = va_arg(ap, const char*);
        if (strval == 0) strval = "<null>";
        goto out_str;
      case 'd':
      case 'i':
        numsig = 1;
        if (flags.sz_halfhalf)
          numval = va_arg(ap, /* signed char, int8_t */ int32_t);
        else if (flags.sz_half)
          numval = va_arg(ap, /* short int, int16_t */ int32_t);
        else if (flags.sz_long)
          numval = va_arg(ap, int32_t);
        else if (flags.sz_longlong)
          numval = va_arg(ap, int64_t);
        else if (flags.sz_max)
          numval = va_arg(ap, intmax_t);
        else if (flags.sz_sizet)
          numval = va_arg(ap, size_t);
        else if (flags.sz_ptrdiff)
          numval = va_arg(ap, ptrdiff_t);
        else if (flags.sz_longdbl)
          goto invalid_fmt;
        else
          numval = va_arg(ap, int);
        goto out_num;
      case 'X':
        upcaseout = 1;
        numbase = 16;
        goto get_num;
      case 'p':
        flags.fl_altfmt = 1;
        numsig = 0;
        numval = (uintptr_t)va_arg(ap, void*);
        goto out_num;
      case 'x':
        numbase = 16;
        goto get_num;
      case 'o':
        numbase = 8;
        /* @suppress("No break at end of case") */
      case 'u':
get_num:
        numsig = 0;
        if (flags.sz_halfhalf)
          numval = va_arg(ap, /* unsigned char, uint8_t */ uint32_t);
        else if (flags.sz_half)
          numval = va_arg(ap, /* unsigned short int, uint16_t */ uint32_t);
        else if (flags.sz_long)
          numval = va_arg(ap, /* unsigned long int */ uint32_t);
        else if (flags.sz_longlong)
          numval = va_arg(ap, /* unsigned long long int*/ uint64_t);
        else if (flags.sz_max)
          numval = va_arg(ap, uintmax_t);
        else if (flags.sz_sizet)
          numval = va_arg(ap, size_t);
        else if (flags.sz_ptrdiff)
          numval = va_arg(ap, ptrdiff_t);
        else if (flags.sz_longdbl)
          goto invalid_fmt;
        else
          numval = va_arg(ap, unsigned int);
        goto out_num;
      case 'n': {
        void *ptrval = va_arg(ap, void*);
        if (ptrval == 0) goto next_format;
        if (flags.sz_halfhalf)
          *((signed char*)ptrval) = out_len;
        else if (flags.sz_half)
          *((/* short int */ int16_t*)ptrval) = out_len;
        else if (flags.sz_long)
          *((/* long int */ int32_t*)ptrval) = out_len;
        else if (flags.sz_longlong)
          *((/* long long int */ int64_t*)ptrval) = out_len;
        else if (flags.sz_max)
          *((intmax_t*)ptrval) = out_len;
        else if (flags.sz_sizet)
          *((size_t*)ptrval) = out_len;
        else if (flags.sz_ptrdiff)
          *((ptrdiff_t*)ptrval) = out_len;
        else if (flags.sz_longdbl)
          goto next_format;
        else
          *((int*)ptrval) = out_len;
        goto next_format;
      }
      default:
        format--;
        break;
    }

invalid_fmt:
    format = fmt_start;

next_format:
    continue;

out_num:
    strval = longlong_to_string(numbuf, sizeof(numbuf),
                                numval, numbase,
                                numsig, flags.fl_showsign, upcaseout);
    if (flags.fl_altfmt) {
      if (numbase == 8) {
        printf_putc(str, &size, &out_len, '0');
        width -= 1;
      } else if (numbase == 16) {
        printf_putc(str, &size, &out_len, '0');
        printf_putc(str, &size, &out_len, upcaseout ? 'X' : 'x');
        width -= 2;
      }
    }
    goto out_str;

out_str:

    int pad = width - strlen(strval);
    if (!flags.fl_leftfmt && (flags.fl_leadzero || flags.fl_leadspace)) {
      c = flags.fl_leadzero ? '0' : ' ';
      while (pad-- > 0) printf_putc(str, &size, &out_len, c);
    }
    while (*strval != 0) printf_putc(str, &size, &out_len, *strval++);
    if (flags.fl_leftfmt) {
      while (pad-- > 0) printf_putc(str, &size, &out_len, ' ');
    }
    goto next_format;
  }

  if (str != 0 && out_len != -1) str[out_len] = 0;
  return out_len;
}

int vprintf(const char *format, va_list ap) {
  va_list tmp;
  va_copy(tmp, ap);
  int len = vsnprintf(0, 0, format, tmp);
  va_end(tmp);
  char buf[len + 1];
  len = vsnprintf(buf, len, format, ap);
  buf[len] = 0;
  puts(buf);
  return len;
}

int printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  int len = vprintf(format, args);
  va_end(args);
  return len;
}
int snprintf(char *str, size_t size, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int len = vsnprintf(str, size, format, args);
  va_end(args);
  return len;
}

size_t strlen(const char* c, size_t limit) {
  const char *e = c;
  while (((size_t)(e - c) < limit) && (*e++) != 0) {}
  return e - c - 1;
}

char* strdup(const char* c) {
  size_t len = strlen(c);
  char* r = (char*)Memory::alloc(len + 1);
  Memory::copy(r, (void*)c, len + 1);
  return r;
}

int strcmp(const char* a, const char* b) {
  while ((*a != 0) && (*b != 0) && (*a++) == (*b++)) {}
  return *a - *b;
}

extern "C" {
  void __cxa_pure_virtual() {
    while (1) {}
  }
  extern char __init_start__, __init_end__;
}

void static_init() {
  for (void (**p)() = (void(**)())&__init_start__;
      p < (void (**)())&__init_end__; ++p)
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
  bool ret_val = 0, new_val = 0;
  asm volatile("lock xchgb %1,%2":
      "=a"(ret_val):
      "r"(new_val),"m"(state):
      "memory");
}
