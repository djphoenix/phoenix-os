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

#include "kernlib.hpp"
#include "acpi.hpp"
#include "processmanager.hpp"

class SMP {
 private:
  static Mutex startupMutex;
  static void NORETURN startup();
  static void init();
};

Mutex SMP::startupMutex;

void SMP::startup() {
  Interrupts::loadVector();
  ACPI::getController()->activateCPU();
  startupMutex.lock();
  startupMutex.release();
  process_loop();
}

void SMP::init() {
  ACPI* acpi = ACPI::getController();
  uint32_t localId = acpi->getLapicID();
  uint32_t cpuCount = acpi->getCPUCount();
  if (cpuCount < 2) return;

  struct StartupInfo {
    DTREG *gdtptr;
    const void *pagetableptr;
    const void *lapicAddr;
    uint64_t *cpuids;
    const char **stacks;
    void(*startup)();
  } PACKED;

  const char *smp_init, *smp_end;
  asm volatile("lea _smp_init(%%rip), %q0":"=r"(smp_init));
  asm volatile("lea _smp_end(%%rip), %q0":"=r"(smp_end));

  const size_t smp_init_size = smp_end - smp_init;

  char *startupCode;
  StartupInfo *info;

  startupCode = static_cast<char*>(Pagetable::alloc());
  info = reinterpret_cast<StartupInfo*>(startupCode + ALIGN(smp_init_size, 8));

  Memory::copy(startupCode, smp_init, smp_init_size);
  char smp_init_vector = (((uintptr_t)startupCode) >> 12) & 0xFF;

  info->gdtptr = new DTREG();
  info->lapicAddr = acpi->getLapicAddr();
  info->cpuids = new uint64_t[cpuCount]();
  info->stacks = new const char*[cpuCount]();
  info->startup = startup;

  asm volatile("mov %%cr3, %q0":"=r"(info->pagetableptr));
  asm volatile("sgdt %0":"=m"(*info->gdtptr):"m"(*info->gdtptr));

  uint32_t nullcpus = 0;
  for (uint32_t i = 0; i < cpuCount; i++) {
    info->cpuids[i] = acpi->getLapicIDOfCPU(i);
    if (info->cpuids[i] != localId)
      info->stacks[i] = static_cast<char*>(Pagetable::alloc()) + 0x1000;
    else
      nullcpus++;
  }
  if (nullcpus > 0)
    nullcpus--;

  startupMutex.lock();

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
    asm volatile("hlt");
  }

  startupMutex.release();

  delete info->gdtptr;
  delete info->cpuids;
  delete info->stacks;
  Pagetable::free(startupCode);
}
