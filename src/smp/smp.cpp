//    PhoeniX OS SMP Subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "acpi.hpp"
#include "interrupts.hpp"
#include "pagetable.hpp"
#include "scheduler.hpp"
#include "syscall.hpp"
#include "memop.hpp"

class SMP {
 private:
  static void setup();
  static void init();
};

void SMP::setup() {
  {
    Interrupts::loadVector();
    ACPI::getController()->activateCPU();
    Syscall::setup();
  }
  asm volatile("jmp *%0"::"r"(Scheduler::process_loop));
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
    const uint8_t **stacks;
    void(*startup)();
    DTREG gdtptr;
  } __attribute__((packed));

  const uint8_t *smp_init, *smp_end;
  asm volatile("lea _smp_init(%%rip), %q0; lea _smp_end(%%rip), %q1":"=X"(smp_init),"=X"(smp_end));

  const size_t smp_init_size = size_t(smp_end - smp_init);
  const size_t cpuids_size = sizeof(uint64_t) * cpuCount;
  const size_t stacks_size = sizeof(uint8_t*) * cpuCount;

  uint8_t *startupCode;
  StartupInfo *info;

  startupCode = static_cast<uint8_t*>(Pagetable::lowalloc(1, Pagetable::MemoryType::CODE_RW));
  info = reinterpret_cast<StartupInfo*>(startupCode + smp_init_size);

  Memory::copy(startupCode, smp_init, smp_init_size);
  uint8_t smp_init_vector = (uintptr_t(startupCode) >> 12) & 0xFF;

  info->lapicAddr = acpi->getLapicAddr();
  info->cpuids = reinterpret_cast<uint64_t*>(Pagetable::alloc((cpuids_size + 0xFFF) >> 12, Pagetable::MemoryType::DATA_RW));
  info->stacks = reinterpret_cast<const uint8_t**>(Pagetable::alloc((stacks_size + 0xFFF) >> 12, Pagetable::MemoryType::DATA_RW));
  info->startup = setup;

  asm volatile("mov %%cr3, %q0":"=X"(info->pagetableptr));
  asm volatile("sgdtq %0":"+m"(info->gdtptr));

  uint32_t nullcpus = 0;
  for (uint32_t i = 0; i < cpuCount; i++) {
    info->cpuids[i] = acpi->getLapicIDOfCPU(uint8_t(i));
    if (info->cpuids[i] != localId)
      info->stacks[i] = static_cast<uint8_t*>(Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW)) + 0x1000;
    else
      nullcpus++;
  }
  Pagetable::map(startupCode, Pagetable::MemoryType::CODE_RX);
  if (nullcpus > 0)
    nullcpus--;

  {
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

  for (size_t i = 0; i < sizeof((cpuids_size + 0xFFF) >> 12); i += 0x1000) {
    Pagetable::free(info->cpuids + (i / sizeof(*info->cpuids)));
  }
  for (size_t i = 0; i < sizeof((stacks_size + 0xFFF) >> 12); i += 0x1000) {
    Pagetable::free(info->stacks + (i / sizeof(*info->stacks)));
  }
  Pagetable::free(startupCode);
}
