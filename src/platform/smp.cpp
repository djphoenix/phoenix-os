//    PhoeniX OS SMP Subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "kernlib.hpp"
#include "acpi.hpp"
#include "processmanager.hpp"
#include "syscall.hpp"

class SMP {
 private:
  static Mutex startupMutex;
  static void setup();
  static void NORETURN startup();
  static void init();
};

Mutex SMP::startupMutex;

void SMP::setup() {
  Interrupts::loadVector();
  ACPI::getController()->activateCPU();
  Syscall::setup();
  Mutex::Lock lock(startupMutex);
}

void SMP::startup() {
  setup();
  ProcessManager::process_loop();
}

void SMP::init() {
  ACPI* acpi = ACPI::getController();
  uint32_t localId = acpi->getLapicID();
  uint32_t cpuCount = acpi->getCPUCount();
  if (cpuCount < 2) return;

  struct StartupInfo {
    const void *pagetableptr;
    const void *lapicAddr;
    uint64_t *cpuids;
    const char **stacks;
    void(*startup)();
    DTREG gdtptr;
  } PACKED;

  const char *smp_init, *smp_end;
  asm volatile("lea _smp_init(%%rip), %q0":"=r"(smp_init));
  asm volatile("lea _smp_end(%%rip), %q0":"=r"(smp_end));

  const size_t smp_init_size = size_t(smp_end - smp_init);

  char *startupCode;
  StartupInfo *info;

  startupCode = static_cast<char*>(Pagetable::alloc());
  info = reinterpret_cast<StartupInfo*>(startupCode + klib::__align(smp_init_size, 8));

  Memory::copy(startupCode, smp_init, smp_init_size);
  uint8_t smp_init_vector = (uintptr_t(startupCode) >> 12) & 0xFF;

  info->lapicAddr = acpi->getLapicAddr();
  info->cpuids = new uint64_t[cpuCount]();
  info->stacks = new const char*[cpuCount]();
  info->startup = startup;

  asm volatile("mov %%cr3, %q0":"=r"(info->pagetableptr));
  asm volatile("sgdt %0":"=m"(info->gdtptr):"m"(info->gdtptr));

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

  {
    Mutex::Lock lock(startupMutex);
    for (uint32_t i = 0; i < cpuCount; i++) {
      if (info->cpuids[i] != localId) {
        acpi->sendCPUInit(uint32_t(info->cpuids[i]));
      }
    }
    while (cpuCount - nullcpus != acpi->getActiveCPUCount()) {
      for (uint32_t i = 0; i < cpuCount; i++) {
        if (info->cpuids[i] != localId) {
          acpi->sendCPUStartup(uint32_t(info->cpuids[i]), smp_init_vector);
        }
      }
      asm volatile("hlt");
    }
  }

  delete info->cpuids;
  delete info->stacks;
  Pagetable::free(startupCode);
}
