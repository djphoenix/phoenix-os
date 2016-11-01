//    PhoeniX OS ACPI subsystem
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

#pragma once
#include "pxlib.hpp"
#include "memory.hpp"
#include "cpu.hpp"

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

struct AcpiHeader {
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
struct AcpiMadt {
  AcpiHeader header;
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

struct ioapic_redir {
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

class ACPI {
 private:
  AcpiMadt *madt;
  uint32_t* localApicAddr, *ioApicAddr;
  uint8_t ioApicMaxCount;
  uint32_t acpiCpuIds[256];
  uint8_t acpiCpuCount;
  static uint8_t activeCpuCount;
  static uint64_t busfreq;
  static ACPI *controller;
  bool ParseRsdp(char* rsdp);
  void ParseRsdt(AcpiHeader* rsdt);
  void ParseXsdt(AcpiHeader* xsdt);
  void ParseDT(AcpiHeader* dt);
  void ParseApic(AcpiMadt *madt);
  void initIOAPIC();
  void initCPU();

 public:
  ACPI();
  static ACPI* getController();
  uint32_t getLapicID();
  uint32_t getCPUID();
  void* getLapicAddr();
  uint32_t getCPUCount();
  uint32_t getActiveCPUCount();
  uint32_t LapicIn(uint32_t reg);
  void LapicOut(uint32_t reg, uint32_t data);
  uint32_t IOapicIn(uint32_t reg);
  void IOapicOut(uint32_t reg, uint32_t data);
  void IOapicMap(uint32_t idx, ioapic_redir r);
  ioapic_redir IOapicReadMap(uint32_t idx);
  void activateCPU();
  uint32_t getLapicIDOfCPU(uint32_t cpuId);
  uint32_t getCPUIDOfLapic(uint32_t lapicId);
  void sendCPUInit(uint32_t id);
  void sendCPUStartup(uint32_t id, uint8_t vector);
  bool initAPIC();
  static void EOI();
};
