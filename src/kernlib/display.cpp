//    PhoeniX OS Kernel library display functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

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
        pos += 2;
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
      case PixelFormatRGBX: case PixelFormatBGRX: return 4;
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
      const uint8_t *fontsym = fb_font_8x16 + (size_t(c) * 16);
      size_t offsetX = offset % charWidth();
      size_t offsetY = offset / charWidth();
      size_t pb = pixelBytes();

      uint8_t *fbptr = static_cast<uint8_t*>(framebuffer)
          + offsetY * 16 * width * pb
          + offsetX * 8 * pb;

      for (size_t y = 0; y < 16; y++) {
        uint8_t *line = fbptr + y * width * pb;
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
    for (uintptr_t ptr = uintptr_t(framebuffer);
        ptr < uintptr_t(framebuffer) + bufferSize() + 0xFFF;
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
    Port<port + 1>::out<uint8_t>(0);
    // Enable DLAB
    Port<port + 3>::out<uint8_t>(0x80);
    // Set divisor
    Port<port + 0>::out<uint8_t>(divisor & 0xFF);
    Port<port + 1>::out<uint8_t>((divisor >> 16) & 0xFF);
    // Set port mode (8N1), disable DLAB
    Port<port + 3>::out<uint8_t>(0x03);
  }
  void write(const char *str) {
    uint64_t t = EnterCritical();
    mutex.lock();
    char c;
    while ((c = *str++) != 0) Port<port>::out<uint8_t>(c);
    mutex.release();
    LeaveCritical(t);
  }
  void clean() {}
};

char *const ConsoleDisplay::base = reinterpret_cast<char*>(0xB8000);
char *const ConsoleDisplay::top = reinterpret_cast<char*>(0xB8FA0);
const size_t ConsoleDisplay::size = ConsoleDisplay::top - ConsoleDisplay::base;

static Display *getSerialDisplay();

static SerialDisplay serialConsole;
Display *Display::instance = getSerialDisplay();
Mutex Display::instanceMutex;

static Display *getSerialDisplay() { return &serialConsole; }

void Display::setup() {
  if (instance != &serialConsole) return;
  const struct EFI::SystemTable *ST = EFI::getSystemTable();
  if (ST) {  // EFI Framebuffer
    EFI::GraphicsOutput *graphics_output = 0;
    ST->BootServices->LocateProtocol(
        &EFI::GUID_GraphicsOutputProtocol, 0,
        reinterpret_cast<void**>(&graphics_output));
    PixelFormat pixelFormat;
    switch (graphics_output->Mode->Info->PixelFormat) {
      case EFI::GRAPHICS_PIXEL_FORMAT_RGBX_8BPP:
        pixelFormat = PixelFormatRGBX;
        break;
      case EFI::GRAPHICS_PIXEL_FORMAT_BGRX_8BPP:
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
