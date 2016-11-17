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
#include "efi.hpp"

class ConsoleDisplay: public Display {
 private:
  static char *const base;
  static char *const top;
  static const size_t size;
  char *display;
  void putc(const char c);
 public:
  ConsoleDisplay();
  void write(const char *str);
  void clean();
};

class EFIDisplay: public Display {
 public:
  EFIDisplay();
  void write(const char *str);
  void clean();
};

class SerialDisplay: public Display {
 public:
  SerialDisplay() {
    // TODO: setup serial
  }
  void write(const char *str) {
    char c;
    while ((c = *str++) != 0) {
      outportb(0x3F8, c);
    }
  }
  void clean() {}
};

char *const ConsoleDisplay::base = reinterpret_cast<char*>(0xB8000);
char *const ConsoleDisplay::top = reinterpret_cast<char*>(0xB8FA0);
const size_t ConsoleDisplay::size = ConsoleDisplay::top - ConsoleDisplay::base;

ConsoleDisplay::ConsoleDisplay() {
  if (EFI::getSystemTable() != 0) return;
  display = base;
  clean();
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
  Memory::fill(base, 0, size);
  mutex.release();
}

EFIDisplay::EFIDisplay() {
  const EFI_SYSTEM_TABLE *ST = EFI::getSystemTable();
  ST->ConOut->Reset(ST->ConOut, 1);
  uint64_t cols = 0, rows = 0, bestmode = 0;
  for (uint64_t m = 0; m < ST->ConOut->Mode->MaxMode; m++) {
    uint64_t c = 0, r = 0;
    ST->ConOut->QueryMode(ST->ConOut, m, &c, &r);
    if (c > cols || r > rows) {
      cols = c; rows = r; bestmode = m;
    }
  }
  if (ST->ConOut->Mode->Mode != bestmode)
    ST->ConOut->SetMode(ST->ConOut, bestmode);
  ST->ConOut->SetAttribute(ST->ConOut, {{ EFI_COLOR_WHITE, EFI_COLOR_BLACK }});
}

void EFIDisplay::clean() {
  const EFI_SYSTEM_TABLE *ST = EFI::getSystemTable();
  mutex.lock();
  ST->ConOut->ClearScreen(ST->ConOut);
  mutex.release();
}

void EFIDisplay::write(const char *str) {
  size_t wlen = 0;
  const char *s = str; char c;
  while ((c = *s++) != 0)
    wlen += (c == '\r') ? 0 : ((c == '\n') ? 2 : 1);
  wchar_t *wstr =
      reinterpret_cast<wchar_t*>(alloca((wlen + 1) * sizeof(wchar_t))),
      *ws = wstr;
  s = str;
  while ((c = *s++) != 0) {
    if (c == '\r') continue;
    if (c == '\n') *ws++ = '\r';
    *ws++ = c;
  }
  *ws++ = 0;

  const EFI_SYSTEM_TABLE *ST = EFI::getSystemTable();
  mutex.lock();
  ST->ConOut->OutputString(ST->ConOut, wstr);
  mutex.release();
}

static SerialDisplay serialConsole;
Display *Display::instance = &serialConsole;
Mutex Display::instanceMutex;

void Display::setup() {
  if (instance != &serialConsole) return;
  if (EFI::getSystemTable()) {
    instance = new EFIDisplay();
  } else {
    instance = new ConsoleDisplay();
  }
}

Display *Display::getInstance() {
  if (instance) return instance;
  instanceMutex.lock();
  if (!instance) setup();
  instanceMutex.release();
  return instance;
}

Display::~Display() {}
