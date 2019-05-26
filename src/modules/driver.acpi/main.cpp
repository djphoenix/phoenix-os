//    PhoeniX OS Stub hello module
//    Copyright © 2019 Yury Popov a.k.a. PhoeniX

#include "kernmod.hpp"

MODDESC(name, "Driver/ACPI");
MODDESC(version, "1.0");
MODDESC(description, "Generic ACPI Driver");
MODDESC(requirements, "");
MODDESC(developer, "PhoeniX");

template<typename T>
static inline size_t putn(T x, uint8_t base = 10, size_t len = 1) {
  char buf[sizeof(T) * 4];
  size_t sz = sizeof(buf) - 1, off = sz;
  buf[off] = 0;
  while (off >= sz + 1 - len || x > 0) {
    uint8_t c = uint8_t(x % base); x /= base;
    buf[--off] = (c < 10) ? static_cast<char>('0' + c) : static_cast<char>('a' - 10 + c);
  }
  puts(buf + off);
  return sz - off;
}

namespace ACPI {
  struct RSDP {
    static const uint64_t SIG = 0x2052545020445352;  // 'RSD PTR '
    uint64_t sig;
    uint8_t crc;
    char oem[6];
    uint8_t rev;
    uintptr_t rsdt:32;

    inline bool _isValid0() const {
      if (sig != SIG) return 0;
      uint8_t sum = 0;
      const uint8_t *buf = reinterpret_cast<const uint8_t*>(this);
      for (size_t i = 0; i < sizeof(RSDP); i++) sum += buf[i];
      if (sum != 0) return 0;
      return 1;
    }
    inline bool isValid0() const {
      return rev == 0 && _isValid0();
    }
  } __attribute__((packed));
  struct RSDPv20 : RSDP {
    size_t size:32;
    uintptr_t xsdt;
    uint8_t extcrc;
    uint8_t rsvd[3];
    inline bool isValid2() const {
      if (!RSDP::_isValid0()) return 0;
      if (rev == 0) return 0;
      uint8_t sum = 0;
      const uint8_t *buf = reinterpret_cast<const uint8_t*>(this);
      for (size_t i = 0; i < sizeof(RSDPv20); i++) sum += buf[i];
      if (sum != 0) return 0;
      return 1;
    }
  } __attribute__((packed));
  struct SDT {
    uint32_t sig;
    uint32_t size;
    uint8_t rev;
    uint8_t crc;
    char oemid[6];
    char oemtblid[8];
    uint32_t oemrev;
    uint32_t crid;
    uint32_t crrev;

    inline bool isValid() const {
      uint8_t sum = 0;
      const uint8_t *buf = reinterpret_cast<const uint8_t*>(this);
      for (size_t i = 0; i < size; i++) sum += buf[i];
      return sum == 0;
    }
    inline size_t data_size() const {
      return size - sizeof(SDT);
    }
  } __attribute__((packed));
  struct RSDT : SDT {
    static const uint32_t SIG = 0x54445352;
    uint32_t ptrs[];
    inline size_t count() const {
      return SDT::data_size() / sizeof(*ptrs);
    }
  } __attribute__((packed));
  struct XSDT : SDT {
    static const uint32_t SIG = 0x54445358;
    uint64_t ptrs[];
    inline size_t count() const {
      return SDT::data_size() / sizeof(*ptrs);
    }
  } __attribute__((packed));

  static void parseDT(const void *addr) {
    SDT desc;
    kread(&desc, addr, sizeof(desc));
    char path[] = "acpi/table.XXXX";
    path[11] = static_cast<char>((desc.sig >> 0) & 0xFF);
    path[12] = static_cast<char>((desc.sig >> 8) & 0xFF);
    path[13] = static_cast<char>((desc.sig >> 16) & 0xFF);
    path[14] = static_cast<char>((desc.sig >> 24) & 0xFF);
    ioprovide(path, addr);
  }
  static void parseXSDT(const void *addr) {
    XSDT desc;
    kread(&desc, addr, sizeof(desc));
    if (desc.sig != XSDT::SIG) return;
    XSDT *xsdt = reinterpret_cast<XSDT*>(__builtin_alloca(desc.size));
    kread(xsdt, addr, desc.size);
    if (!xsdt->isValid()) return;
    for (size_t i = 0; i < xsdt->count(); i++) {
      parseDT(reinterpret_cast<const void*>(xsdt->ptrs[i]));
    }
  }
  static void parseRSDT(const void *addr) {
    RSDT desc;
    kread(&desc, addr, sizeof(desc));
    if (desc.sig != RSDT::SIG) return;
    RSDT *rsdt = reinterpret_cast<RSDT*>(__builtin_alloca(desc.size));
    kread(rsdt, addr, desc.size);
    if (!rsdt->isValid()) return;
    for (size_t i = 0; i < rsdt->count(); i++) {
      parseDT(reinterpret_cast<const void*>(uintptr_t(rsdt->ptrs[i])));
    }
  }
  static void parseRSDP(const void *addr) {
    RSDPv20 desc;
    kread(&desc, addr, sizeof(desc));

    if (desc.isValid2()) {
      parseXSDT(reinterpret_cast<const void*>(desc.xsdt));
    } else if (desc.isValid0()) {
      parseRSDT(reinterpret_cast<const void*>(desc.rsdt));
    }
  }
}  // namespace ACPI

extern "C" {
  extern const char __attribute__((weak)) kptr_acpi_rsdp[];
};

void module() {
  if (kptr_acpi_rsdp == nullptr) {
    puts("No ACPI RSDP\n");
  } else {
    ACPI::parseRSDP(kptr_acpi_rsdp);
  }
  exit(0);
}
