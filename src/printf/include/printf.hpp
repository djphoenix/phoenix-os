//    PhoeniX OS Kernel library printf functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib/std.hpp"

int printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
int vprintf(const char *format, va_list ap);
