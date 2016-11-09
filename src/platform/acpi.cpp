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

#include "acpi.hpp"

#include "cpu.hpp"
#include "pagetable.hpp"

ACPI* ACPI::controller = 0;
uint8_t ACPI::activeCpuCount = 0;
uint64_t ACPI::busfreq = 0;

static void *const ACPI_FIND_START = reinterpret_cast<char*>(0x000e0000);
static void *const ACPI_FIND_TOP = reinterpret_cast<char*>(0x000fffff);
static const uint64_t ACPI_SIG_RTP_DSR = 0x2052545020445352;
static const uint32_t ACPI_SIG_CIPA = 0x43495041;

static Mutex controllerMutex;
ACPI* ACPI::getController() {
  if (controller) return controller;
  uint64_t t = EnterCritical();
  controllerMutex.lock();
  if (!controller)
    controller = new ACPI();
  controllerMutex.release();
  LeaveCritical(t);
  return controller;
}

ACPI::ACPI() {
  uint64_t *p = static_cast<uint64_t*>(ACPI_FIND_START);
  uint64_t *end = static_cast<uint64_t*>(ACPI_FIND_TOP);
  acpiCpuCount = 0;
  activeCpuCount = 1;

  if (!((CPU::getFeatures() >> 32) & CPUID_FEAT_APIC)) {
    acpiCpuCount = 1;
    return;
  }

  while (p < end) {
    Pagetable::map(p);
    Pagetable::map(p + 1);
    uint64_t signature = *p;

    if ((signature == ACPI_SIG_RTP_DSR) && ParseRsdp(p))
      break;

    p += 2;
  }

  outportb(0x61, (inportb(0x61) & 0xFD) | 1);
  outportb(0x43, 0xB2);
  outportb(0x42, 0x9B);
  inportb(0x60);
  outportb(0x42, 0x2E);
  uint8_t t = inportb(0x61) & 0xFE;
  outportb(0x61, t);
  outportb(0x61, t | 1);
  LapicOut(LAPIC_TMRINITCNT, -1);
  while ((inportb(0x61) & 0x20) == 0) {}
  LapicOut(LAPIC_LVT_TMR, LAPIC_DISABLE);
  busfreq = ((-1 - LapicIn(LAPIC_TMRCURRCNT)) << 4) * 100;
}
void ACPI::ParseDT(AcpiHeader *header) {
  Pagetable::map(header);

  if (header->signature == ACPI_SIG_CIPA)
    ParseApic(reinterpret_cast<AcpiMadt*>(header));
}
void ACPI::ParseRsdt(AcpiHeader *rsdt) {
  Pagetable::map(rsdt);
  uint32_t *p = reinterpret_cast<uint32_t*>(rsdt + 1);
  uint32_t *end = p + (rsdt->length - sizeof(*rsdt)) / sizeof(uint32_t);

  while (p < end) {
    Pagetable::map(p);
    Pagetable::map(p + 1);
    uintptr_t address = ((*p++) & 0xFFFFFFFF);
    ParseDT(reinterpret_cast<AcpiHeader*>(address));
  }
}
void ACPI::ParseXsdt(AcpiHeader *xsdt) {
  Pagetable::map(xsdt);
  uint64_t *p = reinterpret_cast<uint64_t*>(xsdt + 1);
  uint64_t *end = p + (xsdt->length - sizeof(*xsdt)) / sizeof(uint64_t);

  while (p < end) {
    Pagetable::map(p);
    Pagetable::map(p + 1);
    uint64_t address = *p++;
    ParseDT(reinterpret_cast<AcpiHeader*>(address));
  }
}
void ACPI::ParseApic(AcpiMadt *a_madt) {
  Pagetable::map(a_madt);
  madt = a_madt;

  localApicAddr = reinterpret_cast<char*>(madt->localApicAddr);
  Pagetable::map(localApicAddr);

  uint8_t *p = reinterpret_cast<uint8_t*>(madt + 1);
  uint8_t *end = p + (madt->header.length - sizeof(AcpiMadt));

  while (p < end) {
    ApicHeader *header = reinterpret_cast<ApicHeader*>(p);
    Pagetable::map(header);
    Pagetable::map(header + 1);
    uint16_t type = header->type;
    uint16_t length = header->length;
    Pagetable::map(header);
    if (type == 0) {
      ApicLocalApic *s = reinterpret_cast<ApicLocalApic*>(p);
      acpiCpuIds[acpiCpuCount++] = s->apicId;
    } else if (type == 1) {
      ApicIoApic *s = reinterpret_cast<ApicIoApic*>(p);
      ioApicAddr = reinterpret_cast<char*>(s->ioApicAddress);
      Pagetable::map(ioApicAddr);
    } else if (type == 2) {
      ApicInterruptOverride *s = reinterpret_cast<ApicInterruptOverride*>(p);
      (void)s;
      // TODO: handle interrupt overrides
    }

    p += length;
  }
}
bool ACPI::ParseRsdp(void *ptr) {
  uint8_t *p = static_cast<uint8_t*>(ptr);

  uint8_t sum = 0;
  for (int i = 0; i < 20; ++i)
    sum += p[i];

  if (sum)
    return false;

  char oem[7];
  Memory::copy(oem, p + 9, 6);
  oem[6] = 0;

  char revision = p[15];
  uint32_t *rsdtPtr = static_cast<uint32_t*>(ptr) + 4;
  uint64_t *xsdtPtr = static_cast<uint64_t*>(ptr) + 3;

  Pagetable::map(rsdtPtr);
  Pagetable::map(rsdtPtr + 1);
  uint32_t rsdtAddr = *rsdtPtr;
  if (revision == 0) {
    ParseRsdt(reinterpret_cast<AcpiHeader*>(rsdtAddr));
  } else if (revision == 2) {
    Pagetable::map(xsdtPtr);
    Pagetable::map(xsdtPtr + 1);
    uint64_t xsdtAddr = *xsdtPtr;

    if (xsdtAddr)
      ParseXsdt(reinterpret_cast<AcpiHeader*>(xsdtAddr));
    else
      ParseRsdt(reinterpret_cast<AcpiHeader*>(rsdtAddr));
  }

  return true;
}
uint32_t ACPI::getLapicID() {
  if (!localApicAddr)
    return 0;
  return LapicIn(LAPIC_APICID) >> 24;
}
uint32_t ACPI::getCPUID() {
  if (!localApicAddr)
    return 0;
  return getCPUIDOfLapic(getLapicID());
}
uint32_t ACPI::getLapicIDOfCPU(uint32_t id) {
  if (!localApicAddr)
    return 0;
  return acpiCpuIds[id];
}
uint32_t ACPI::getCPUIDOfLapic(uint32_t id) {
  if (!localApicAddr)
    return 0;
  for (uint32_t i = 0; i < 256; i++) {
    if (acpiCpuIds[i] == id)
      return i;
  }
  return 0;
}
uint32_t ACPI::LapicIn(uint32_t reg) {
  if (!localApicAddr)
    return 0;
  return MmioRead32(localApicAddr + reg);
}
void ACPI::LapicOut(uint32_t reg, uint32_t data) {
  if (!localApicAddr)
    return;
  MmioWrite32(localApicAddr + reg, data);
}
uint32_t ACPI::IOapicIn(uint32_t reg) {
  if (!ioApicAddr)
    return 0;
  MmioWrite32(ioApicAddr + IOAPIC_REGSEL, reg);
  return MmioRead32(ioApicAddr + IOAPIC_REGWIN);
}
void ACPI::IOapicOut(uint32_t reg, uint32_t data) {
  if (!ioApicAddr)
    return;
  MmioWrite32(ioApicAddr + IOAPIC_REGSEL, reg);
  MmioWrite32(ioApicAddr + IOAPIC_REGWIN, data);
}
union ioapic_redir_ints {
  ioapic_redir r;
  uint32_t i[2];
};
void ACPI::IOapicMap(uint32_t idx, ioapic_redir r) {
  ioapic_redir_ints a = { .r = r };
  IOapicOut(IOAPIC_REDTBL + idx * 2 + 0, a.i[0]);
  IOapicOut(IOAPIC_REDTBL + idx * 2 + 1, a.i[1]);
}
ioapic_redir ACPI::IOapicReadMap(uint32_t idx) {
  ioapic_redir_ints a;
  a.i[0] = IOapicIn(IOAPIC_REDTBL + idx * 2 + 0);
  a.i[1] = IOapicIn(IOAPIC_REDTBL + idx * 2 + 1);
  return a.r;
}
void* ACPI::getLapicAddr() {
  return localApicAddr;
}
uint32_t ACPI::getCPUCount() {
  if (!localApicAddr)
    return 1;
  return acpiCpuCount;
}
uint32_t ACPI::getActiveCPUCount() {
  if (!localApicAddr)
    return 1;
  return activeCpuCount;
}

void ACPI::initCPU() {
  LapicOut(LAPIC_DFR, 0xFFFFFFFF);
  LapicOut(LAPIC_LDR, (LapicIn(LAPIC_LDR) & 0x00FFFFFF) | 1);
  LapicOut(LAPIC_LVT_TMR, LAPIC_DISABLE);
  LapicOut(LAPIC_LVT_PERF, LAPIC_NMI);
  LapicOut(LAPIC_LVT_LINT0, LAPIC_DISABLE);
  LapicOut(LAPIC_LVT_LINT1, LAPIC_DISABLE);
  LapicOut(LAPIC_TASKPRIOR, 0);

  asm volatile("rdmsr; bts $11,%%eax; wrmsr"::"c"(0x1B):"rax");

  LapicOut(LAPIC_SPURIOUS, 0x27 | LAPIC_SW_ENABLE);

  uint64_t c = (ACPI::busfreq / 1000) >> 4;
  if (c < 0x10)
    c = 0x10;

  LapicOut(LAPIC_TMRINITCNT, c & 0xFFFFFFFF);
  LapicOut(LAPIC_TMRDIV, 3);
  LapicOut(LAPIC_LVT_TMR, 0x20 | TMR_PERIODIC);
}

void ACPI::activateCPU() {
  if (!localApicAddr)
    return;
  initCPU();
  uint64_t ret_val;
  asm volatile("lock incq %1":"=a"(ret_val):"m"(activeCpuCount):"memory");
  EOI();
}
void ACPI::sendCPUInit(uint32_t id) {
  if (!localApicAddr)
    return;
  LapicOut(LAPIC_ICRH, id << 24);
  LapicOut(LAPIC_ICRL, 0x00004500);
  while (LapicIn(LAPIC_ICRL) & 0x00001000) {}
}
void ACPI::sendCPUStartup(uint32_t id, uint8_t vector) {
  if (!localApicAddr)
    return;
  LapicOut(LAPIC_ICRH, id << 24);
  LapicOut(LAPIC_ICRL, vector | 0x00004600);
  while (LapicIn(LAPIC_ICRL) & 0x00001000) {}
}
void ACPI::EOI() {
  if (!(ACPI::getController())->localApicAddr)
    return;
  (ACPI::getController())->LapicOut(LAPIC_EOI, 0);
}
void ACPI::initIOAPIC() {
  if (ioApicAddr == 0)
    return;
  ioApicMaxCount = (IOapicIn(IOAPIC_VER) >> 16) & 0xFF;
  ioapic_redir kbd;
  kbd.vector = 0x21;
  kbd.deliveryMode = 1;
  kbd.destinationMode = 0;
  kbd.deliveryStatus = 0;
  kbd.pinPolarity = 0;
  kbd.remoteIRR = 0;
  kbd.triggerMode = 0;
  kbd.mask = 0;
  kbd.destination = 0x00;
  IOapicMap(1, kbd);
}
bool ACPI::initAPIC() {
  if (!((CPU::getFeatures() >> 32) & CPUID_FEAT_APIC))
    return false;
  if (localApicAddr == 0)
    return false;
  initCPU();
  initIOAPIC();
  EOI();
  return true;
}
