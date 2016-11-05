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

struct GDT {
  GDT_ENT ents[5];
  GDT_SYS_ENT sys_ents[];

  static size_t size(size_t sys_count) {
    return sizeof(GDT_ENT) * 5 + sizeof(GDT_SYS_ENT) * sys_count;
  }
} PACKED;

GDT *gdt = 0;
TSS64_ENT *tss = 0;
DTREG gdtrec = {0, 0};

void SMP::init_gdt(uint32_t ncpu) {
  gdt = new(GDT::size(ncpu)) GDT();
  tss = new TSS64_ENT[ncpu]();

  gdtrec.addr = gdt;
  gdtrec.limit = GDT::size(ncpu) - 1;
  gdt->ents[0] = GDT_ENT();
  gdt->ents[1] = GDT_ENT(0, 0, 0xA, 0, 1, 1, 0, 1, 0, 0);
  gdt->ents[2] = GDT_ENT(0, 0, 0x2, 0, 1, 1, 0, 1, 0, 0);
  gdt->ents[3] = GDT_ENT(0, 0xFFFFFFFFFFFFFFFF, 0xA, 3, 1, 1, 0, 1, 0, 0);
  gdt->ents[4] = GDT_ENT(0, 0xFFFFFFFFFFFFFFFF, 0x2, 3, 1, 1, 0, 1, 0, 0);
  for (uint32_t idx = 0; idx < ncpu; idx++) {
    void *stack = Memory::palloc();
    uintptr_t stack_ptr = (uintptr_t)stack + 0x1000;
    Memory::zero(&tss[idx], sizeof(tss[idx]));
    tss[idx].ist[0] = stack_ptr;
    gdt->sys_ents[idx] = GDT_SYS_ENT(
        (uintptr_t)&tss[idx], sizeof(TSS64_ENT),
        0x9, 0, 0, 1, 0, 1, 0, 0);
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