//    PhoeniX OS ACPI subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once

#include <stdint.h>
#include "mutex.hpp"

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

  static ACPI controller;

  uint8_t *localApicAddr, *ioApicAddr;
  const void *rsdpAddr;
  uint8_t ioApicMaxCount;
  uint8_t acpiCpuLapicIds[256];
  uint8_t acpiLapicCpuIds[256];
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
  void init();

 public:
  inline static ACPI* getController() { return &controller; }
  static void setup();
  uint8_t getLapicID();
  uint8_t getCPUID();
  const void* getRSDPAddr() __attribute__((pure));
  void* getLapicAddr() __attribute__((pure));
  uint32_t getCPUCount() __attribute__((pure));
  uint32_t getActiveCPUCount() __attribute__((pure));
  uint32_t LapicIn(uint32_t reg);
  void LapicOut(uint32_t reg, uint32_t data);
  uint32_t IOapicIn(uint32_t reg);
  void IOapicOut(uint32_t reg, uint32_t data);
  void IOapicMap(uint32_t idx, const IOApicRedir &r);
  IOApicRedir IOapicReadMap(uint32_t idx);
  void activateCPU();
  uint8_t getLapicIDOfCPU(uint8_t cpuId) __attribute__((pure));
  uint8_t getCPUIDOfLapic(uint8_t lapicId) __attribute__((pure));
  void sendCPUInit(uint32_t id);
  void sendCPUStartup(uint32_t id, uint8_t vector);
  bool initAPIC();
  static void EOI();
};
