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
#include "pagetable.hpp"
#include "font-8x16.hpp"

class ConsoleDisplay: public Display {
 private:
  static char *const base;
  static char *const top;
  static const size_t size;
  char *display;
  void putc(const char c) {
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

 public:
  ConsoleDisplay() {
    if (EFI::getSystemTable() != 0) return;
    display = base;
    clean();
  }
  void write(const char *str) {
    uint64_t t = EnterCritical();
    mutex.lock();
    while (*str != 0) putc(*(str++));
    mutex.release();
    LeaveCritical(t);
  }
  void clean() {
    mutex.lock();
    Memory::fill(base, 0, size);
    mutex.release();
  }
};

enum PixelFormat {
  PixelFormatRGBX,
  PixelFormatBGRX
};

class FramebufferDisplay: public Display {
 private:
  void *framebuffer;
  size_t width, height;
  PixelFormat pixelFormat;
  size_t offset;

  size_t pixelBytes() {
    switch (pixelFormat) {
      case PixelFormatRGBX: return 4;
      case PixelFormatBGRX: return 4;
    }
    return 0;
  }
  size_t bufferSize() { return width * height * pixelBytes(); }

  size_t charWidth() { return width / 8; }
  size_t charHeight() { return height / 16; }

  void putc(const char c) {
    if (c == 0) return;
    if (c == '\n') {
      offset += charWidth() - (offset % charWidth());
    } else if (c == '\t') {
      offset += 8 - ((offset % charWidth()) % 8);
    } else {
      const uint8_t *fontsym = fb_font_8x16 + ((size_t)c * 16);
      size_t offsetX = offset % charWidth();
      size_t offsetY = offset / charWidth();
      size_t pb = pixelBytes();

      char *fbptr = static_cast<char*>(framebuffer)
          + offsetY * 16 * width * pb
          + offsetX * 8 * pb;

      for (size_t y = 0; y < 16; y++) {
        char *line = fbptr + y * width * pb;
        uint8_t fontline = fontsym[y];
        for (size_t x = 0; x < 8; x++) {
          if (fontline & (1 << (8 - x))) {
            for (size_t px = 0; px < pb; px++)
              line[x * pb + px] = 0xFF;
          }
        }
      }

      offset++;
    }
    if (offset >= charHeight() * charWidth()) {
      char *fbptr = static_cast<char*>(framebuffer);
      size_t pb = pixelBytes();
      Memory::copy(fbptr,
                   fbptr + width * 16 * pb,
                   bufferSize() - width * 16 * pb);
      Memory::zero(fbptr + width * 16 * (charHeight() - 1) * pb,
                   width * 16 * pb);
      offset -= charWidth();
    }
  }

 public:
  FramebufferDisplay(void *framebuffer, size_t width, size_t height,
                     PixelFormat pixelFormat):
    framebuffer(framebuffer), width(width), height(height),
    pixelFormat(pixelFormat), offset(0) {
    for (uintptr_t ptr = (uintptr_t)framebuffer;
        ptr < (uintptr_t)framebuffer + bufferSize() + 0xFFF;
        ptr += 0x1000) {
      Pagetable::map(reinterpret_cast<void*>(ptr));
    }
    clean();
  }
  void write(const char *str) {
    uint64_t t = EnterCritical();
    mutex.lock();
    while (*str != 0) putc(*(str++));
    mutex.release();
    LeaveCritical(t);
  }
  void clean() {
    mutex.lock();
    Memory::zero(framebuffer, bufferSize());
    mutex.release();
  }
};

class SerialDisplay: public Display {
 private:
  static const uint16_t port = 0x3F8;
  static const uint16_t baud = 9600;
  static const uint16_t divisor = 115200 / baud;

 public:
  SerialDisplay() {
    // Disable interrupts
    outportb(port + 1, 0);
    // Enable DLAB
    outportb(port + 3, 0x80);
    // Set divisor
    outportb(port + 0, divisor & 0xFF);
    outportb(port + 1, (divisor >> 16) & 0xFF);
    // Set port mode (8N1), disable DLAB
    outportb(port + 3, 0x03);
  }
  void write(const char *str) {
    uint64_t t = EnterCritical();
    mutex.lock();
    char c;
    while ((c = *str++) != 0) outportb(port, c);
    mutex.release();
    LeaveCritical(t);
  }
  void clean() {}
};

char *const ConsoleDisplay::base = reinterpret_cast<char*>(0xB8000);
char *const ConsoleDisplay::top = reinterpret_cast<char*>(0xB8FA0);
const size_t ConsoleDisplay::size = ConsoleDisplay::top - ConsoleDisplay::base;

static SerialDisplay serialConsole;
Display *Display::instance = &serialConsole;
Mutex Display::instanceMutex;

void Display::setup() {
  if (instance != &serialConsole) return;
  const EFI_SYSTEM_TABLE *ST = EFI::getSystemTable();
  if (ST) {  // EFI Framebuffer
    EFI_GRAPHICS_OUTPUT *graphics_output = 0;
    ST->BootServices->LocateProtocol(
        &EFI_GRAPHICS_OUTPUT_PROTOCOL, 0,
        reinterpret_cast<void**>(&graphics_output));
    PixelFormat pixelFormat;
    switch (graphics_output->Mode->Info->PixelFormat) {
      case EFI_GRAPHICS_PIXEL_FORMAT_RGBX_8BPP:
        pixelFormat = PixelFormatRGBX;
        break;
      case EFI_GRAPHICS_PIXEL_FORMAT_BGRX_8BPP:
        pixelFormat = PixelFormatBGRX;
        break;
      default:
        return;
    }
    instance = new FramebufferDisplay(
        graphics_output->Mode->FrameBufferBase,
        graphics_output->Mode->Info->HorizontalResolution,
        graphics_output->Mode->Info->VerticalResolution,
        pixelFormat);
    return;
  }

  // Fallback
  instance = new ConsoleDisplay();
}

Display *Display::getInstance() {
  if (instance) return instance;
  uint64_t t = EnterCritical();
  instanceMutex.lock();
  if (!instance) setup();
  instanceMutex.release();
  LeaveCritical(t);
  return instance;
}

Display::~Display() {}
