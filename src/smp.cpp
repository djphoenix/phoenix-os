//    PhoeniX OS SMP Subsystem
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

#include "smp.hpp"
#include "process.hpp"

struct GDT_ENT {
  uint64_t seg_lim_low :16;
  uint64_t base_low :24;
  uint8_t type :4;
  bool system :1;
  uint8_t dpl :2;
  bool present :1;
  uint64_t seg_lim_high :4;
  bool avl :1;
  bool islong :1;
  bool db :1;
  bool granularity :1;
  uint64_t base_high :8;
} PACKED;

struct GDT_SYS_ENT {
  GDT_ENT ent;
  uint64_t base_high :32;
  uint32_t rsvd;
} PACKED;

struct {
  uint16_t size;
  uintptr_t base;
} PACKED gdtrec;

struct GDT_ENT_def {
  uint64_t base, limit;
  uint8_t type, dpl;
  bool system, present, avl;
  bool islong, db, granularity;
};

inline GDT_ENT gdt_encode(GDT_ENT_def def) {
  struct GDT_ENT res;
  res.seg_lim_low = def.limit & 0xFFFF;
  res.base_low = def.base & 0xFFFFFF;
  res.type = def.type;
  res.system = def.system;
  res.dpl = def.dpl;
  res.present = def.present;
  res.seg_lim_high = (def.limit >> 16) & 0xF;
  res.avl = def.avl;
  res.islong = def.islong;
  res.db = def.db;
  res.granularity = def.granularity;
  res.base_high = (def.base >> 24) & 0xFF;
  return res;
}

inline GDT_ENT_def gdt_decode(GDT_ENT ent) {
  struct GDT_ENT_def res;
  res.base = ((uint64_t)ent.base_high << 24) | ent.base_low;
  res.limit = ((uint64_t)ent.seg_lim_high << 16) | ent.seg_lim_low;
  res.type = ent.type;
  res.dpl = ent.dpl;
  res.system = ent.system;
  res.present = ent.present;
  res.avl = ent.avl;
  res.islong = ent.islong;
  res.db = ent.db;
  res.granularity = ent.granularity;
  return res;
}

inline GDT_SYS_ENT gdt_sys_encode(GDT_ENT_def def) {
  struct GDT_SYS_ENT res;
  res.ent = gdt_encode(def);
  res.base_high = def.base >> 32;
  res.rsvd = 0;
  return res;
}

inline GDT_ENT_def gdt_sys_decode(GDT_SYS_ENT ent_sys) {
  struct GDT_ENT_def res;
  res = gdt_decode(ent_sys.ent);
  res.base |= (ent_sys.base_high << 32);
  return res;
}

struct TSS64_ENT {
  uint32_t reserved1;
  uint64_t rsp[3];
  uint64_t reserved2;
  uint64_t ist[7];
  uint64_t reserved3;
  uint16_t reserved4;
  uint16_t iomap_base;
} PACKED;

static const GDT_ENT GDT_ENT_zero = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

GDT_ENT *gdt = 0;
TSS64_ENT *tss = 0;

void SMP::init_gdt(uint32_t ncpu) {
  size_t size = 5 * sizeof(GDT_ENT) + ncpu * sizeof(GDT_SYS_ENT);
  gdt = Memory::alloc<GDT_ENT>(size);
  GDT_SYS_ENT *gdtsys = reinterpret_cast<GDT_SYS_ENT*>(&gdt[5]);
  tss = new TSS64_ENT[ncpu]();

  gdtrec.base = (uintptr_t)gdt;
  gdtrec.size = size - 1;
  gdt[0] = GDT_ENT_zero;
  gdt[1] = gdt_encode({ 0, 0, 0xA, 0, 1, 1, 0, 1, 0, 0 });
  gdt[2] = gdt_encode({ 0, 0, 0x2, 0, 1, 1, 0, 1, 0, 0 });
  gdt[3] = gdt_encode({ 0, 0xFFFFFFFFFFFFFFFF, 0xA, 3, 1, 1, 0, 1, 0, 0 });
  gdt[4] = gdt_encode({ 0, 0xFFFFFFFFFFFFFFFF, 0x2, 3, 1, 1, 0, 1, 0, 0 });
  for (uint32_t idx = 0; idx < ncpu; idx++) {
    void *stack = Memory::palloc();
    uintptr_t stack_ptr = (uintptr_t)stack + 0x1000;
    Memory::zero(&tss[idx], sizeof(tss[idx]));
    tss[idx].ist[0] = stack_ptr;
    gdtsys[idx] = gdt_sys_encode({
        (uintptr_t)&tss[idx], sizeof(TSS64_ENT), 0x9, 0, 0, 1, 0, 1, 0, 0 });
  }
}

void SMP::setup_gdt() {
  ACPI* acpi = ACPI::getController();
  if (gdt == 0)
    init_gdt(acpi->getCPUCount());
  uint32_t cpuid = acpi->getCPUID();
  uint16_t tr = 5 * sizeof(GDT_ENT) + cpuid * sizeof(GDT_SYS_ENT);
  uint64_t t = EnterCritical();
  asm volatile("lgdtq %0; ltr %w1"::"m"(gdtrec), "a"(tr));
  LeaveCritical(t);
}

Mutex cpuinit = Mutex();

void SMP::startup() {
  setup_gdt();
  ACPI::getController()->activateCPU();
  cpuinit.lock();
  cpuinit.release();
  Interrupts::loadVector();
  process_loop();
}

extern "C" {
  extern char _smp_init, _smp_end;
}

void SMP::init() {
  ACPI* acpi = ACPI::getController();
  uint32_t localId = acpi->getLapicID();
  uint32_t cpuCount = acpi->getCPUCount();
  if (cpuCount == 1) {
    setup_gdt();
    return;
  }

  struct StartupInfo {
    const void *lapicAddr;
    uint64_t *cpuids;
    const char **stacks;
    void(*startup)();
  } PACKED;

  const size_t smp_init_size = &_smp_end - &_smp_init;

  char *startupCode;
  StartupInfo *info;

  startupCode = static_cast<char*>(Memory::palloc());
  info = reinterpret_cast<StartupInfo*>(startupCode + ALIGN(smp_init_size, 8));

  Memory::copy(startupCode, &_smp_init, smp_init_size);
  char smp_init_vector = (((uintptr_t)startupCode) >> 12) & 0xFF;

  info->lapicAddr = acpi->getLapicAddr();
  info->cpuids = new uint64_t[cpuCount]();
  info->stacks = new const char*[cpuCount]();
  info->startup = startup;

  setup_gdt();
  uint32_t nullcpus = 0;
  for (uint32_t i = 0; i < cpuCount; i++) {
    info->cpuids[i] = acpi->getLapicIDOfCPU(i);
    if (info->cpuids[i] != localId)
      info->stacks[i] = static_cast<char*>(Memory::palloc()) + 0x1000;
    else
      nullcpus++;
  }
  if (nullcpus > 0)
    nullcpus--;

  cpuinit.lock();

  for (uint32_t i = 0; i < cpuCount; i++) {
    if (info->cpuids[i] != localId) {
      acpi->sendCPUInit(info->cpuids[i]);
    }
  }
  while (cpuCount - nullcpus != acpi->getActiveCPUCount()) {
    for (uint32_t i = 0; i < cpuCount; i++) {
      if (info->cpuids[i] != localId) {
        acpi->sendCPUStartup(info->cpuids[i], smp_init_vector);
      }
    }
  }

  cpuinit.release();

  delete[] info->cpuids;
  delete[] info->stacks;
  Memory::pfree(startupCode);
}
