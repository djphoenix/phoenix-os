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
  struct Header;
  struct Madt;
  struct ApicHeader;
  struct ApicLocalApic;
  struct ApicIoApic;
  struct ApicInterruptOverride;

  static Mutex controllerMutex;
  static volatile ACPI *controller;

  char *localApicAddr, *ioApicAddr;
  const void *rsdpAddr;
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
  const void* getRSDPAddr() PURE;
  void* getLapicAddr() PURE;
  uint32_t getCPUCount() PURE;
  uint32_t getActiveCPUCount() PURE;
  uint32_t LapicIn(uint32_t reg);
  void LapicOut(uint32_t reg, uint32_t data);
  uint32_t IOapicIn(uint32_t reg);
  void IOapicOut(uint32_t reg, uint32_t data);
  void IOapicMap(uint32_t idx, const IOApicRedir &r);
  IOApicRedir IOapicReadMap(uint32_t idx);
  void activateCPU();
  uint32_t getLapicIDOfCPU(uint32_t cpuId) PURE;
  uint32_t getCPUIDOfLapic(uint32_t lapicId) PURE;
  void sendCPUInit(uint32_t id);
  void sendCPUStartup(uint32_t id, uint8_t vector);
  bool initAPIC();
  static void EOI();
};
