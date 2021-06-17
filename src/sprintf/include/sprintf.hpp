//    PhoeniX OS Kernel library printf functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once

#include <stddef.h>
#include <stdarg.h>

int snprintf(char *str, size_t size, const char *format, ...)
  __attribute__((format(printf, 3, 4)));

int vsnprintf(char *str, size_t size, const char *format, va_list ap);
