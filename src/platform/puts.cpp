#include "kernlib/std.hpp"
#include "kernlib/mutex.hpp"
#include "kernlib/ports.hpp"

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
      while ((Port<port + 5>::in8() & (1 << 5)) == 0) ;
      Port<port>::out8(uint8_t(c));
    }
  }
};

void SerialConsole::setup() {
  // Disable interrupts
  Port<port + 1>::out8(0);
  // Enable DLAB
  Port<port + 3>::out8(0x80);
  // Set divisor
  Port<port + 0>::out8(divisor & 0xFF);
  Port<port + 1>::out8((divisor >> 16) & 0xFF);
  // Set port mode (8N1), disable DLAB
  Port<port + 3>::out8(0x03);
}

SerialConsole SerialConsole::instance;

extern "C" {
  void puts(const char *str) {
    SerialConsole::instance.write(str);
  }
}
