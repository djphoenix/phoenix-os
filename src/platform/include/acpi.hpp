//    PhoeniX OS ACPI subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"

class ACPI {
 public:
  enum LAPIC_FIELDS {
    LAPIC_APICID = 0x20,
    LAPIC_APICVER = 0x30,
    LAPIC_TASKPRIOR = 0x80,
    LAPIC_EOI = 0xB0,
    LAPIC_LDR = 0xD0,
    LAPIC_DFR = 0xE0,
    LAPIC_SPURIOUS = 0xF0,
    LAPIC_ESR = 0x280,
    LAPIC_ICRL = 0x300,
    LAPIC_ICRH = 0x310,
    LAPIC_LVT_TMR = 0x320,
    LAPIC_LVT_PERF = 0x340,
    LAPIC_LVT_LINT0 = 0x350,
    LAPIC_LVT_LINT1 = 0x360,
    LAPIC_LVT_ERR = 0x370,
    LAPIC_TMRINITCNT = 0x380,
    LAPIC_TMRCURRCNT = 0x390,
    LAPIC_TMRDIV = 0x3E0,
    LAPIC_LAST = 0x38F,
  };

 private:
  enum LAPIC_VALUES {
    LAPIC_DISABLE = 0x10000,
    LAPIC_SW_ENABLE = 0x100,
    LAPIC_CPUFOCUS = 0x200,
    LAPIC_NMI = (4 << 8),
    TMR_PERIODIC = 0x20000,
    TMR_BASEDIV = (1 << 20),
  };

  enum IOAPIC_FIELDS {
    IOAPIC_REGSEL = 0x0, IOAPIC_REGWIN = 0x10,
  };

  enum IOAPIC_REGS {
    IOAPIC_ID = 0x0, IOAPIC_VER = 0x1, IOAPIC_REDTBL = 0x10,
  };

  struct Header {
    uint32_t signature;
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    uint8_t oem[6];
    uint8_t oemTableId[8];
    uint32_t oemRevision;
    uint32_t creatorId;
    uint32_t creatorRevision;
  };
  struct Madt {
    Header header;
    uint32_t localApicAddr;
    uint32_t flags;
  };
  struct ApicHeader {
    uint8_t type;
    uint8_t length;
  };
  struct ApicLocalApic {
    ApicHeader header;
    uint8_t acpiProcessorId;
    uint8_t apicId;
    uint32_t flags;
  };
  struct ApicIoApic {
    ApicHeader header;
    uint8_t ioApicId;
    uint8_t reserved;
    uint32_t ioApicAddress;
    uint32_t globalSystemInterruptBase;
  };
  struct ApicInterruptOverride {
    ApicHeader header;
    uint8_t bus;
    uint8_t source;
    uint32_t interrupt;
    uint16_t flags;
  };

  union IOApicRedir {
    struct {
      uint8_t vector :8;
      uint8_t deliveryMode :3;
      bool destinationMode :1;
      bool deliveryStatus :1;
      bool pinPolarity :1;
      bool remoteIRR :1;
      bool triggerMode :1;
      bool mask :1;
      uint64_t reserved :39;
      uint8_t destination :8;
    };
    uint32_t raw[2];
  };

 private:
  static Mutex controllerMutex;
  static ACPI *controller;

  char *localApicAddr, *ioApicAddr;
  uint8_t ioApicMaxCount;
  uint32_t acpiCpuIds[256];
  uint8_t acpiCpuCount;
  uint8_t activeCpuCount;
  uint64_t busfreq;
  bool ParseRsdp(const void* rsdp);
  void ParseRsdt(const Header* rsdt);
  void ParseXsdt(const Header* xsdt);
  void ParseDT(const Header* dt);
  void ParseApic(const Madt *madt);
  void initIOAPIC();
  void initCPU();

 public:
  ACPI();
  static ACPI* getController();
  uint32_t getLapicID();
  uint32_t getCPUID();
  void* getLapicAddr() PURE;
  uint32_t getCPUCount() PURE;
  uint32_t getActiveCPUCount() PURE;
  uint32_t LapicIn(uint32_t reg);
  void LapicOut(uint32_t reg, uint32_t data);
  uint32_t IOapicIn(uint32_t reg);
  void IOapicOut(uint32_t reg, uint32_t data);
  void IOapicMap(uint32_t idx, IOApicRedir r);
  IOApicRedir IOapicReadMap(uint32_t idx);
  void activateCPU();
  uint32_t getLapicIDOfCPU(uint32_t cpuId) PURE;
  uint32_t getCPUIDOfLapic(uint32_t lapicId) PURE;
  void sendCPUInit(uint32_t id);
  void sendCPUStartup(uint32_t id, uint8_t vector);
  bool initAPIC();
  static void EOI();
};
