//    PhoeniX OS Kernel library printf functions
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

#pragma once
#include "std.hpp"

extern "C" {

  int printf(const char *format, ...)
  __attribute__ ((format (printf, 1, 2)));

  int snprintf(char *str, size_t size, const char *format, ...)
  __attribute__ ((format (printf, 3, 4)));

  int vprintf(const char *format, va_list ap);
  int vsnprintf(char *str, size_t size, const char *format, va_list ap);

}
