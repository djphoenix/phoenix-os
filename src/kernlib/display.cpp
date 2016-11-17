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

inline static void serout(const char *str) {
  char c;
      while ((c = *str++) != 0) outportb(0x3F8, c);
}

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

  size_t pixelBytes() {
    switch (pixelFormat) {
      case PixelFormatRGBX: return 4;
      case PixelFormatBGRX: return 4;
    }
    return 0;
  }
  size_t bufferSize() { return width * height * pixelBytes(); }

 public:
  FramebufferDisplay(void *framebuffer, size_t width, size_t height,
                     PixelFormat pixelFormat):
    framebuffer(framebuffer), width(width), height(height),
    pixelFormat(pixelFormat) {
    for (uintptr_t ptr = (uintptr_t)framebuffer;
        ptr < (uintptr_t)framebuffer + bufferSize() + 0xFFF;
        ptr += 0x1000) {
      Pagetable::map(reinterpret_cast<void*>(ptr));
    }
    printf("FB: %p, res: %lux%lu\n", framebuffer, width, height);

    clean();
  }
  void write(const char *str) {
    // TODO: Framebuffer print
    serout(str);
  }
  void clean() {
    Memory::zero(framebuffer, bufferSize());
  }
};

class SerialDisplay: public Display {
 public:
  SerialDisplay() {}
  void write(const char *str) {
    uint64_t t = EnterCritical();
    mutex.lock();
    serout(str);
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
  if (ST) {
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
  } else {
    instance = new ConsoleDisplay();
  }
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
