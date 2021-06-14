//    PhoeniX OS Kernel library printf functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "kernlib.hpp"
#include "heap.hpp"

static inline char *longlong_to_string(
    char *buf, size_t len, uint64_t n, uint8_t base,
    bool fl_signed, bool fl_showsign, bool fl_caps) {
  size_t pos = len;
  char table_start = fl_caps ? 'A' : 'a';
  bool negative = 0;
  if (fl_signed && int64_t(n) < 0) {
    negative = 1;
    n = -n;
  }
  buf[ --pos] = 0;
  while (n) {
    uint8_t digit = uint8_t(n % base);
    n /= base;
    if (digit < 10) {
      buf[ --pos] = static_cast<char>(digit + '0');
    } else {
      buf[ --pos] = static_cast<char>(digit - 10 + table_start);
    }
  }
  if (buf[pos] == 0) buf[ --pos] = '0';
  if (negative) buf[ --pos] = '-';
  else if (fl_showsign) buf[ --pos] = '+';
  return &buf[pos];
}

static void printf_putc(char *str, size_t *size, int *len, char c) {
  if ((*len) == -1) return;
  (*len)++;
  if (str == nullptr) return;
  if (*size == 0) {
    *len = -1;
    return;
  }
  (*size)--;
  str[*len-1] = c;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
  union flags_t {
    struct {
      struct {
        uint8_t fl_leftfmt :1;    // %-
        uint8_t fl_showsign :1;   // %+
        uint8_t fl_leadspace :1;  // % [space]
        uint8_t fl_altfmt :1;     // %#
        uint8_t fl_leadzero :1;   // %0
      } PACKED;
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
    } PACKED;
    struct {
      uint32_t raw_flags :5;
      uint32_t raw_size :8;
    } PACKED;
    uint32_t raw;
  } PACKED;

  char c;
  const char *fmt_start = nullptr;
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
        c = static_cast<char>(va_arg(ap, /* char */ int) & 0xFF);
        numbuf[0] = c;
        numbuf[1] = 0;
        strval = numbuf;
        goto out_str;
      case 's':
        strval = va_arg(ap, const char*);
        if (strval == nullptr) strval = "<null>";
        goto out_str;
      case 'd':
      case 'i':
        numsig = 1;
        if (flags.sz_halfhalf)
          numval = uintmax_t(va_arg(ap, /* signed char, int8_t */ int32_t));
        else if (flags.sz_half)
          numval = uintmax_t(va_arg(ap, /* short int, int16_t */ int32_t));
        else if (flags.sz_long)
          numval = uintmax_t(va_arg(ap, int32_t));
        else if (flags.sz_longlong)
          numval = uintmax_t(va_arg(ap, int64_t));
        else if (flags.sz_max)
          numval = uintmax_t(va_arg(ap, intmax_t));
        else if (flags.sz_sizet)
          numval = uintmax_t(va_arg(ap, size_t));
        else if (flags.sz_ptrdiff)
          numval = uintmax_t(va_arg(ap, ptrdiff_t));
        else if (flags.sz_longdbl)
          goto invalid_fmt;
        else
          numval = uintmax_t(va_arg(ap, int));
        goto out_num;
      case 'X':
        upcaseout = 1;
        numbase = 16;
        goto get_num;
      case 'p':
        flags.fl_altfmt = 1;
        numsig = 0;
        numval = uintptr_t(va_arg(ap, void*));
        numbase = 16;
        goto out_num;
      case 'x':
        numbase = 16;
        goto get_num;
      case 'o':
        numbase = 8;
        /* @suppress("No break at end of case") */
        /* fallthrough */
      case 'u':
get_num:
        numsig = 0;
        if (flags.sz_halfhalf)
          numval = va_arg(ap, /* unsigned char, uint8_t */ uint32_t);
        else if (flags.sz_half)
          numval = va_arg(ap, /* unsigned short int, uint16_t */ uint32_t);
        else if (flags.sz_long)
          numval = va_arg(ap, /* unsigned long int */ uint64_t);
        else if (flags.sz_longlong)
          numval = va_arg(ap, /* unsigned long long int*/ uint64_t);
        else if (flags.sz_max)
          numval = va_arg(ap, uintmax_t);
        else if (flags.sz_sizet)
          numval = va_arg(ap, size_t);
        else if (flags.sz_ptrdiff)
          numval = uintmax_t(va_arg(ap, ptrdiff_t));
        else if (flags.sz_longdbl)
          goto invalid_fmt;
        else
          numval = va_arg(ap, uint32_t);
        goto out_num;
      case 'n': {
        void *ptrval = va_arg(ap, void*);
        if (ptrval == nullptr) goto next_format;
        if (flags.sz_halfhalf)
          *static_cast<int8_t*>(ptrval) = int8_t(out_len);
        else if (flags.sz_half)
          *static_cast<int16_t*>(ptrval) = int16_t(out_len);
        else if (flags.sz_long)
          *static_cast<int64_t*>(ptrval) = out_len;
        else if (flags.sz_longlong)
          *static_cast<int64_t*>(ptrval) = out_len;
        else if (flags.sz_max)
          *static_cast<intmax_t*>(ptrval) = out_len;
        else if (flags.sz_sizet)
          *static_cast<size_t*>(ptrval) = size_t(out_len);
        else if (flags.sz_ptrdiff)
          *static_cast<ptrdiff_t*>(ptrval) = out_len;
        else if (flags.sz_longdbl)
          goto next_format;
        else
          *static_cast<int32_t*>(ptrval) = out_len;
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

    int pad = width - static_cast<int>(klib::strlen(strval));
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

  if (str != nullptr && out_len != -1) str[out_len] = 0;
  return out_len;
}

int vprintf(const char *format, va_list ap) {
  va_list tmp;
  va_copy(tmp, ap);
  int len = vsnprintf(nullptr, 0, format, tmp);
  va_end(tmp);
  char smbuf[512], *buf;
  if (len > 511) {
    buf = static_cast<char*>(Heap::alloc(size_t(len) + 1));
  } else {
    buf = smbuf;
  }
  len = vsnprintf(buf, size_t(len), format, ap);
  buf[len] = 0;
  klib::puts(buf);
  if (len > 511) Heap::free(buf);
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
