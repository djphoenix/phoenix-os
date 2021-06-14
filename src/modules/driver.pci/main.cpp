//    PhoeniX OS Stub hello module
//    Copyright © 2019 Yury Popov a.k.a. PhoeniX

#include "kernmod.hpp"

MODDESC(name, "Driver/PCI");
MODDESC(version, "1.0");
MODDESC(description, "Generic PCI Driver");
MODDESC(requirements, "port/CF8-CFF");
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

namespace PCI {
  static inline uint32_t readConfig32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address, value;
    address = 0x80000000u |
        (uint32_t(bus) << 16) |
        (uint32_t(slot) << 11) |
        (uint32_t(func) << 8) |
        (uint32_t(offset) & 0xFC);
    asm volatile(
        "mov $0xCF8, %%dx\n"
        "outl %%eax, %%dx\n"
        "add $4, %%dx\n"
        "inl %%dx, %%eax"
        :"=a"(value):"a"(address):"dx");
    return value;
  }
  struct Device {
    uint16_t vendor, device;
    uint16_t command, status;
    uint8_t revision, prog_if, subclass, classcode;
    uint8_t cache_line_size, latency_timer, header_type, bist;
    union {
      struct {
        uint32_t bar[6];
        uint32_t cardbus_cis;
        uint16_t subsystem_vendor, subsystem_id;
        uint32_t rom_base;
        uint8_t caps_ptr, rsvd[3+4];
        uint8_t intr_line, intr_pin, min_grant, max_latency;
      } __attribute__((packed)) device;
      struct {
        uint32_t bar[2];
        uint8_t prim_bus_num, sec_bus_num, sub_bus_num, sec_latency_timer;
        uint8_t iobase, iolimit; uint16_t sec_status;
        uint16_t membase, memlim;
        uint16_t pref_membase, pref_memlim;
        uint32_t pref_membase_u32;
        uint32_t pref_memlim_u32;
        uint16_t iobase_u16, iolim_u16;
        uint8_t caps_ptr, rsvd[3];
        uint32_t rom_base;
        uint8_t intr_line, intr_pin; uint16_t bridgectl;
      } __attribute__((packed)) pcibridge;
      struct {
        uint32_t cardbus_sock_exca_base;
        uint8_t cap_off, rsvd; uint16_t sec_status;
        uint8_t pci_bus_num, cardbus_bus_num, sub_bus_num, cardbus_lat_timer;
        uint32_t membase0, memlim0;
        uint32_t membase1, memlim1;
        uint32_t iobase0, iolim0;
        uint32_t iobase1, iolim1;
        uint8_t intr_line, intr_pin, bridgectl;
        uint16_t sub_device, sub_vendor;
        uint32_t pccard_base;
      } __attribute__((packed)) cardbusbridge;
    } __attribute__((packed)) _spec;

    template<typename T> static inline T read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
      return T(readConfig32(bus, slot, func, offset) >> ((offset & 0x3) * 8));
    }
    static inline uint8_t getHeaderType(uint8_t bus, uint8_t slot, uint8_t func) {
      return read<uint8_t>(bus, slot, func, offsetof(Device, header_type));
    }
    static inline uint16_t getVendorID(uint8_t bus, uint8_t slot, uint8_t func) {
      return read<uint16_t>(bus, slot, func, offsetof(Device, vendor));
    }
    static inline uint16_t getDeviceID(uint8_t bus, uint8_t slot, uint8_t func) {
      return read<uint16_t>(bus, slot, func, offsetof(Device, device));
    }
    static inline uint8_t getBaseClass(uint8_t bus, uint8_t slot, uint8_t func) {
      return read<uint8_t>(bus, slot, func, offsetof(Device, classcode));
    }
    static inline uint8_t getSubClass(uint8_t bus, uint8_t slot, uint8_t func) {
      return read<uint8_t>(bus, slot, func, offsetof(Device, subclass));
    }
    static inline uint8_t getProgIf(uint8_t bus, uint8_t slot, uint8_t func) {
      return read<uint8_t>(bus, slot, func, offsetof(Device, prog_if));
    }
    static inline uint8_t getRevision(uint8_t bus, uint8_t slot, uint8_t func) {
      return read<uint8_t>(bus, slot, func, offsetof(Device, revision));
    }
    static inline uint8_t getSecondaryBus(uint8_t bus, uint8_t slot, uint8_t func) {
      return read<uint8_t>(bus, slot, func, offsetof(Device, _spec.pcibridge.sec_bus_num));
    }
  } __attribute__((packed));
  static void provideDevice(uint8_t bus, uint8_t device, uint8_t function) {
    uint8_t baseClass = Device::getBaseClass(bus, device, function);
    uint8_t subClass = Device::getSubClass(bus, device, function);
    uint8_t progIf = Device::getProgIf(bus, device, function);
    uint8_t rev = Device::getRevision(bus, device, function);
    uint16_t vendorId = Device::getVendorID(bus, device, function);
    uint16_t deviceId = Device::getDeviceID(bus, device, function);

    uintptr_t address = 0x80000000u |
        (uint32_t(bus) << 16) |
        (uint32_t(device) << 11) |
        (uint32_t(function) << 8);
    const void *ptr = reinterpret_cast<const void*>(address);

    char pathbuf[64];
    snprintf(pathbuf, sizeof(pathbuf), "pci/path.%03d.%03d.%03d", bus, device, function);
    ioprovide(pathbuf, ptr);
    snprintf(pathbuf, sizeof(pathbuf), "pci/class.%02x.%02x.%02x", baseClass, subClass, progIf);
    ioprovide(pathbuf, ptr);
    snprintf(pathbuf, sizeof(pathbuf), "pci/vendor.%04x/device.%04x/rev.%02x", vendorId, deviceId, rev);
    ioprovide(pathbuf, ptr);
  }
  static void scanBus(uint8_t bus);
  static void scanFunction(uint8_t bus, uint8_t device, uint8_t function) {
    uint8_t baseClass = Device::getBaseClass(bus, device, function);
    uint8_t subClass = Device::getSubClass(bus, device, function);
    if ((baseClass == 0x06) && (subClass == 0x04)) {
      uint8_t secondaryBus = Device::getSecondaryBus(bus, device, function);
      scanBus(secondaryBus);
    } else {
      provideDevice(bus, device, function);
    }
  }
  static void scanDevice(uint8_t bus, uint8_t device) {
    uint8_t function = 0;
    uint16_t vendorID = Device::getVendorID(bus, device, function);
    if (vendorID == 0xFFFF) return;
    scanFunction(bus, device, function);
    uint8_t headerType = Device::getHeaderType(bus, device, function);
    if ((headerType & 0x80) != 0) {
      for (function = 1; function < 8; function++) {
        if (Device::getVendorID(bus, device, function) != 0xFFFF) {
          scanFunction(bus, device, function);
        }
      }
    }
  }
  static void scanBus(uint8_t bus) {
    static uint64_t checked = 0;
    if (checked & (1llu << bus)) return;
    checked |= (1llu << bus);
    for (uint8_t device = 0; device < 32; device++) {
      scanDevice(bus, device);
    }
  }
  static void scanAllBuses() {
    uint8_t headerType = Device::getHeaderType(0, 0, 0);
    if ((headerType & 0x80) == 0) {
      scanBus(0);
    } else {
      for (uint8_t function = 0; function < 8; function++) {
        if (Device::getVendorID(0, 0, function) != 0xFFFF) break;
        scanBus(function);
      }
    }
  }
}  // namespace PCI

void module() {
  PCI::scanAllBuses();
  exit(0);
}
