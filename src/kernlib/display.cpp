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

#include "kernlib.hpp"

ConsoleDisplay::ConsoleDisplay() {
  display = base;
}

void ConsoleDisplay::putc(const char c) {
  if (c == 0) return;
  size_t pos = display - base;
  if (c == '\n') {
    display += 160 - (pos % 160);
  } else if (c == '\t') {
    do {
      *(display++) = ' ';
      *(display++) = 0x0F;
    } while (pos % 8 != 0);
  } else {
    *(display++) = c;
    *(display++) = 0x0F;
  }
  if (display >= top) {
    size_t size = top - base;
    Memory::copy(base, base + 160, size - 160);
    display = top - 160;
    Memory::fill(display, 0, 160);
  }
}

void ConsoleDisplay::write(const char *str) {
  uint64_t t = EnterCritical();
  mutex.lock();
  while (*str != 0) putc(*(str++));
  mutex.release();
  LeaveCritical(t);
}

void ConsoleDisplay::clean() {
  mutex.lock();
  size_t size = top - base;
  Memory::fill(base, 0, size);
  mutex.release();
}

static Mutex instanceMutex;
static ConsoleDisplay defaultDisplay;
Display *Display::instance;

Display *Display::getInstance() {
  if (instance) return instance;
  instanceMutex.lock();
  if (!instance) {
    instance = &defaultDisplay;
    // TODO: create dynamic instance
  }
  instanceMutex.release();
  return instance;
}

Display::~Display() {}
