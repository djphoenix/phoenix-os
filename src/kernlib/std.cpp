//    PhoeniX OS Kernel library standard functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "kernlib.hpp"

class SerialConsole {
 private:
  static const uint16_t port = 0x3F8;
  static const uint16_t baud = 9600;
  static const uint16_t divisor = 115200 / baud;
  Mutex mutex;

  static void setup();

 public:
  static SerialConsole instance;
  inline constexpr SerialConsole() {}
  inline void write(const char *str) {
    Mutex::CriticalLock lock(mutex);
    char c;
    while ((c = *str++) != 0) {
      while ((Port<port + 5>::in<uint8_t>() & (1 << 5)) == 0) ;
      Port<port>::out(uint8_t(c));
    }
  }
};

void SerialConsole::setup() {
  // Disable interrupts
  Port<port + 1>::out<uint8_t>(0);
  // Enable DLAB
  Port<port + 3>::out<uint8_t>(0x80);
  // Set divisor
  Port<port + 0>::out<uint8_t>(divisor & 0xFF);
  Port<port + 1>::out<uint8_t>((divisor >> 16) & 0xFF);
  // Set port mode (8N1), disable DLAB
  Port<port + 3>::out<uint8_t>(0x03);
}

SerialConsole SerialConsole::instance;

namespace klib {
  size_t strlen(const char* c, size_t limit) {
    const char *e = c;
    while ((size_t(e - c) < limit) && (*e++) != 0) {}
    return size_t(e - c - 1);
  }

  char* strdup(const char* c) {
    size_t len = strlen(c);
    char* r = new char[len + 1]();
    Memory::copy(r, c, len + 1);
    return r;
  }

  char* strndup(const char* c, size_t len) {
    char* r = new char[len + 1]();
    Memory::copy(r, c, len);
    r[len] = 0;
    return r;
  }

  int strncmp(const char* a, const char* b, int max) {
    size_t i = 0;
    while (i < size_t(max) && a[i] != 0 && b[i] != 0 && a[i] == b[i]) { i++; }
    return (i < size_t(max)) ? a[i] - b[i] : 0;
  }
  int strcmp(const char* a, const char* b) {
    size_t i = 0;
    while (a[i] != 0 && b[i] != 0 && a[i] == b[i]) { i++; }
    return a[i] - b[i];
  }

  void puts(const char *str) {
    SerialConsole::instance.write(str);
  }
}  // namespace klib
