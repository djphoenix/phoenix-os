//    PhoeniX OS Kernel library display functions
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
#include "mutex.hpp"

class Display {
 private:
  static Display *instance;
 public:
  static Display *getInstance();
  virtual void clean() = 0;
  virtual void write(const char*) = 0;
  virtual ~Display() = 0;
};

class ConsoleDisplay: public Display {
 private:
  static constexpr char * const base = reinterpret_cast<char*>(0xB8000);
  static constexpr char * const top = reinterpret_cast<char*>(0xB8FA0);
  char *display;
  void putc(const char c);
  Mutex mutex;
 public:
  ConsoleDisplay();
  void write(const char *str);
  void clean();
};
