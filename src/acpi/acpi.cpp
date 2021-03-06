//    PhoeniX OS ACPI subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "acpi.hpp"

#include "efi.hpp"
#include "cpu.hpp"
#include "pagetable.hpp"

#include "memop.hpp"
#include "portio.hpp"

ACPI ACPI::controller {};

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

struct ACPI::Header {
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
struct ACPI::Madt : Header {
  uint32_t localApicAddr;
  uint32_t flags;
};
struct ACPI::ApicHeader {
  uint8_t type;
  uint8_t length;
};
struct ACPI::ApicLocalApic : ApicHeader {
  uint8_t acpiProcessorId;
  uint8_t apicId;
  uint32_t flags;
};
struct ACPI::ApicIoApic : ApicHeader {
  uint8_t ioApicId;
  uint8_t reserved;
  uint32_t ioApicAddress;
  uint32_t globalSystemInterruptBase;
};
struct ACPI::ApicInterruptOverride : ApicHeader {
  uint8_t bus;
  uint8_t source;
  uint32_t interrupt;
  uint16_t flags;
};

inline static void MmioWrite32(void *p, uint32_t data) {
  *reinterpret_cast<volatile uint32_t *>(p) = data;
}
inline static uint32_t MmioRead32(const void *p) {
  return *reinterpret_cast<const volatile uint32_t *>(p);
}

void ACPI::setup() { controller.init(); }
void ACPI::init() {
  static const void *const ACPI_FIND_START = reinterpret_cast<const void*>(0x000e0000);
  static const void *const ACPI_FIND_TOP = reinterpret_cast<const void*>(0x000fffff);
  static const uint64_t ACPI_SIG_RTP_DSR = 0x2052545020445352;  // 'RTP DSR '

  acpiCpuCount = 0;
  activeCpuCount = 1;

  localApicAddr = nullptr;
  ioApicAddr = nullptr;

  const uint64_t *ptr = nullptr;
  if (nullptr != (ptr = static_cast<const uint64_t*>(EFI::getACPI2Addr()))) {
    Pagetable::map(ptr, ptr + 1, Pagetable::MemoryType::DATA_RO);
    if ((*ptr != ACPI_SIG_RTP_DSR) || !ParseRsdp(ptr)) ptr = nullptr;
  }
  if (!ptr && (ptr = static_cast<const uint64_t*>(EFI::getACPI1Addr()))) {
    Pagetable::map(ptr, ptr + 1, Pagetable::MemoryType::DATA_RO);
    if ((*ptr != ACPI_SIG_RTP_DSR) || !ParseRsdp(ptr)) ptr = nullptr;
  }
  if (!ptr) {
    ptr = static_cast<const uint64_t*>(ACPI_FIND_START);
    const uint64_t *end = static_cast<const uint64_t*>(ACPI_FIND_TOP);
    Pagetable::map(ptr, end, Pagetable::MemoryType::DATA_RO);
    while (ptr < end) {
      if ((*ptr == ACPI_SIG_RTP_DSR) && ParseRsdp(ptr)) break;

      ptr += 2;
    }
    if (ptr >= end) ptr = nullptr;
  }

  if ((CPU::getFeatures() & CPU::CPUID_FEAT_APIC) && !localApicAddr) {
    static const uint32_t msr = 0x1B;
    uintptr_t lapic;
    asm volatile("rdmsr":"=A"(lapic):"c"(msr));
    lapic &= ~0xFFFllu;
    localApicAddr = reinterpret_cast<uint8_t*>(lapic);
    Pagetable::map(localApicAddr, localApicAddr + 0x1000, Pagetable::MemoryType::DATA_RW);
    lapic |= 0x800;
    asm volatile("wrmsr"::"a"(uint32_t(lapic)), "d"(uint32_t(lapic >> 32)), "c"(msr));
    uint8_t lapicId = getLapicID();
    acpiCpuLapicIds[0] = lapicId;
    acpiLapicCpuIds[lapicId] = 0;
    acpiCpuCount = 1;
  }

  if (!localApicAddr) {
    acpiCpuCount = 1;
    return;
  }

  LapicOut(LAPIC_TMRDIV, 3);
  LapicOut(LAPIC_LVT_TMR, 0x20 | LAPIC_SW_ENABLE);
  Port<0x61>::out8((Port<0x61>::in8() & 0xFD) | 1);
  Port<0x43>::out8(0xB2);
  Port<0x42>::out8(0x9B);
  Port<0x60>::in8();
  Port<0x42>::out8(0x2E);
  uint8_t t = Port<0x61>::in8() & 0xFE;
  Port<0x61>::out8(t);
  Port<0x61>::out8(t | 1);
  LapicOut(LAPIC_TMRINITCNT, 0xFFFFFFFF);
  while ((Port<0x61>::in8() & 0x20) == (t & 0x20)) {}
  LapicOut(LAPIC_LVT_TMR, LAPIC_DISABLE);
  busfreq = (static_cast<uint64_t>(0xFFFFFFFF - LapicIn(LAPIC_TMRCURRCNT)) << 4) * 100;
}
void ACPI::ParseDT(const Header *header) {
  Pagetable::map(header, Pagetable::MemoryType::DATA_RO);

  if (header->signature == 0x43495041)  // 'CIPA'
    ParseApic(reinterpret_cast<const Madt*>(header));
}
void ACPI::ParseRsdt(const Header *rsdt) {
  Pagetable::map(rsdt, Pagetable::MemoryType::DATA_RO);
  const uint32_t *p = reinterpret_cast<const uint32_t*>(rsdt + 1);
  const uint32_t *end = p + (rsdt->length - sizeof(*rsdt)) / sizeof(uint32_t);

  while (p < end) {
    Pagetable::map(p, p + 1, Pagetable::MemoryType::DATA_RO);
    uintptr_t address = ((*p++) & 0xFFFFFFFF);
    ParseDT(reinterpret_cast<Header*>(address));
  }
}
void ACPI::ParseXsdt(const Header *xsdt) {
  Pagetable::map(xsdt, Pagetable::MemoryType::DATA_RO);
  uintptr_t p = reinterpret_cast<uintptr_t>(xsdt + 1);
  uintptr_t end = p + (xsdt->length - sizeof(*xsdt));

  while (p < end) {
    const uintptr_t *pp = reinterpret_cast<const uintptr_t*>(p);
    Pagetable::map(pp, pp + 1, Pagetable::MemoryType::DATA_RO);
    ParseDT(reinterpret_cast<Header*>(*pp));
  }
}
void ACPI::ParseApic(const Madt *madt) {
  Pagetable::map(madt, Pagetable::MemoryType::DATA_RO);

  localApicAddr = reinterpret_cast<uint8_t*>(uintptr_t(madt->localApicAddr));
  Pagetable::map(localApicAddr, localApicAddr + 0x1000, Pagetable::MemoryType::DATA_RW);

  uintptr_t p = reinterpret_cast<uintptr_t>(madt + 1);
  uintptr_t end = p + (madt->Header::length - sizeof(Madt));

  while (p < end) {
    const ApicHeader *header = reinterpret_cast<const ApicHeader*>(p);
    Pagetable::map(header, header + 1, Pagetable::MemoryType::DATA_RO);
    uint16_t type = header->type;
    uint16_t length = header->length;
    if (type == 0) {
      const ApicLocalApic *s = reinterpret_cast<const ApicLocalApic*>(p);
      acpiLapicCpuIds[s->apicId] = acpiCpuCount;
      acpiCpuLapicIds[acpiCpuCount] = s->apicId;
      acpiCpuCount++;
    } else if (type == 1) {
      const ApicIoApic *s = reinterpret_cast<const ApicIoApic*>(p);
      ioApicAddr = reinterpret_cast<uint8_t*>(uintptr_t(s->ioApicAddress));
      Pagetable::map(ioApicAddr, ioApicAddr + 0x1000, Pagetable::MemoryType::DATA_RW);
    } else if (type == 2) {
      const ApicInterruptOverride *s = reinterpret_cast<const ApicInterruptOverride*>(p);
      (void)s;
      // TODO: handle interrupt overrides
    }

    p += length;
  }
}
bool ACPI::ParseRsdp(const void *ptr) {
  const uint8_t *p = static_cast<const uint8_t*>(ptr);

  uint8_t sum = 0;
  for (int i = 0; i < 20; ++i)
    sum += p[i];

  if (sum)
    return false;

  rsdpAddr = ptr;

  char oem[7];
  Memory::copy(oem, p + 9, 6);
  oem[6] = 0;

  uint8_t revision = p[15];
  const uint32_t *rsdtPtr = static_cast<const uint32_t*>(ptr) + 4;
  const uint64_t *xsdtPtr = static_cast<const uint64_t*>(ptr) + 3;

  Pagetable::map(rsdtPtr, rsdtPtr + 1, Pagetable::MemoryType::DATA_RO);
  uint32_t rsdtAddr = *rsdtPtr;
  if (revision == 0) {
    ParseRsdt(reinterpret_cast<Header*>(uintptr_t(rsdtAddr)));
  } else if (revision == 2) {
    Pagetable::map(xsdtPtr, xsdtPtr + 1, Pagetable::MemoryType::DATA_RO);
    uint64_t xsdtAddr = *xsdtPtr;

    if (xsdtAddr)
      ParseXsdt(reinterpret_cast<Header*>(xsdtAddr));
    else
      ParseRsdt(reinterpret_cast<Header*>(uintptr_t(rsdtAddr)));
  }

  return true;
}
uint8_t ACPI::getLapicID() {
  if (!localApicAddr)
    return 0;
  return uint8_t(LapicIn(LAPIC_APICID) >> 24);
}
uint8_t ACPI::getCPUID() {
  if (!localApicAddr)
    return 0;
  return getCPUIDOfLapic(getLapicID());
}
uint8_t ACPI::getLapicIDOfCPU(uint8_t id) {
  if (!localApicAddr)
    return 0;
  return acpiCpuLapicIds[id];
}
uint8_t ACPI::getCPUIDOfLapic(uint8_t id) {
  if (!localApicAddr)
    return 0;
  return acpiLapicCpuIds[id];
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
void ACPI::IOapicMap(uint32_t idx, const IOApicRedir &r) {
  IOapicOut(IOAPIC_REDTBL + idx * 2 + 0, r.raw[0]);
  IOapicOut(IOAPIC_REDTBL + idx * 2 + 1, r.raw[1]);
}
ACPI::IOApicRedir ACPI::IOapicReadMap(uint32_t idx) {
  IOApicRedir a;
  a.raw[0] = IOapicIn(IOAPIC_REDTBL + idx * 2 + 0);
  a.raw[1] = IOapicIn(IOAPIC_REDTBL + idx * 2 + 1);
  return a;
}
const void* ACPI::getRSDPAddr() {
  return rsdpAddr;
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

  asm volatile("rdmsr; bts $11,%%eax; wrmsr"::"c"(0x1B):"rax","cc");

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
  if (ioApicAddr == nullptr)
    return;
  ioApicMaxCount = (IOapicIn(IOAPIC_VER) >> 16) & 0xFF;
  IOApicRedir kbd;
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
  if (localApicAddr == nullptr)
    return false;
  initCPU();
  initIOAPIC();
  EOI();
  return true;
}
