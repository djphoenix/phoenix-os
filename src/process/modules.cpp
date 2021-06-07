//    PhoeniX OS Modules subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "modules.hpp"

#include "readelf.hpp"
#include "multiboot_info.hpp"

volatile ModuleManager* ModuleManager::manager = nullptr;
Mutex ModuleManager::managerMutex;

bool ModuleManager::parseModuleInfo(ModuleInfo *info, const Process *process) {
  info->name = nullptr;
  info->version = nullptr;
  info->description = nullptr;
  info->requirements = nullptr;
  info->developer = nullptr;

  struct {
    uintptr_t name, version, desc, reqs, dev;
  } symbols = {
    process->getSymbolByName("module_name"),
    process->getSymbolByName("module_version"),
    process->getSymbolByName("module_description"),
    process->getSymbolByName("module_requirements"),
    process->getSymbolByName("module_developer")
  };

  if ((symbols.name == 0) || (symbols.version == 0)
      || (symbols.desc == 0) || (symbols.reqs == 0) || (symbols.dev == 0))
    return false;

  info->name = process->readString(symbols.name);
  info->version = process->readString(symbols.version);
  info->description = process->readString(symbols.desc);
  info->requirements = process->readString(symbols.reqs);
  info->developer = process->readString(symbols.dev);

  return true;
}

template<typename T> static inline bool readHexChar(char c, T *inout) {
  if (c >= '0' && c <= '9') return (*inout = T(*inout << 4) | T(c - '0'), 1);
  if (c >= 'a' && c <= 'f') return (*inout = T(*inout << 4) | T(c - 'a' + 10), 1);
  if (c >= 'A' && c <= 'F') return (*inout = T(*inout << 4) | T(c - 'A' + 10), 1);
  return 0;
}

static inline bool parsePort(const char *str, uint16_t *min, uint16_t *max) {
  uint16_t *port = min;
  char c;
  while ((c = *str++) != 0) {
    if (c == '-' && port == min) { port = max; continue; }
    if (!readHexChar(c, port)) return 0;
  }
  if (port == min) *max = *min;
  return *max >= *min;
}

enum cpuid_reg { REG_eax, REG_ebx, REG_ecx, REG_edx };
static inline bool parseCPUID(const char *str, uint32_t *func, enum cpuid_reg *reg, uint8_t *bit) {
  char c;
  while ((c = *str++) != 0 && c != '.') {
    if (!readHexChar(c, func)) return 0;
  }
  if (c != '.') return 0;
  switch (*str++) {
    case 'a': case 'A': *reg = REG_eax; break;
    case 'b': case 'B': *reg = REG_ebx; break;
    case 'c': case 'C': *reg = REG_ecx; break;
    case 'd': case 'D': *reg = REG_edx; break;
    default: return 0;
  }
  if ((*str++) != '.') return 0;
  while ((c = *str++) != 0) {
    if (!readHexChar(c, bit)) return 0;
  }
  return 1;
}

bool ModuleManager::bindRequirement(const char *req, Process *process) {
  if (klib::strncmp(req, "port/", 5) == 0) {
    uint16_t minport = 0, maxport = 0;
    if (!parsePort(req + 5, &minport, &maxport)) return 0;
    process->allowIOPorts(minport, maxport);
    return 1;
  } else if (klib::strncmp(req, "cpuid/", 6) == 0) {
    uint32_t func = 0; uint8_t bit = 0; enum cpuid_reg reg;
    if (!parseCPUID(req + 6, &func, &reg, &bit)) return 0;
    if (func & 0x0000FFFF) {
      uint32_t maxfunc = 0;
      asm volatile("cpuid":"=a"(maxfunc):"a"(uint32_t(func & 0xFFFF0000)):"ecx", "ebx", "edx");
      if (func > maxfunc) return 0;
    }
    union {
      uint32_t regs[4];
      struct { uint32_t eax, ebx, ecx, edx; };
    } val;
    asm volatile("cpuid":"=a"(val.eax), "=b"(val.ebx), "=c"(val.ecx), "=d"(val.edx):"a"(func));
    return ((val.regs[reg] >> bit) & 1);
  } else {
    return 0;
  }
}

bool ModuleManager::bindRequirements(const char *reqs, Process *process) {
  if (reqs == nullptr) return 1;
  const char *re;
  ptr<char> r;
  while (*reqs != 0) {
    if (*reqs == ';') { reqs++; continue; }
    re = reqs;
    while (*re != 0 && *re != ';') re++;
    r = klib::strndup(reqs, size_t(re - reqs));
    reqs = re;

    if (!bindRequirement(r.get(), process)) {
      printf("Unsatisfied requirement: %s\n", r.get());
      return 0;
    }
  }
  return 1;
}

void ModuleManager::loadMemory(const void *mem, size_t memsize) {
  size_t size;
  ModuleInfo mod;
  while (memsize > 0) {
    ptr<Process> process { new Process() };
    size = readelf(process.get(), mem, memsize);
    if (size == 0 || !parseModuleInfo(&mod, process.get())) {
      break;
    }
    process->setName(mod.name.get());
    if (bindRequirements(mod.requirements.get(), process.get())) {
      process.release()->startup();
    }

    mem = reinterpret_cast<const uint8_t*>(mem) + size;
    memsize -= size;
  }
}
extern "C" {
  extern const uint8_t __modules_start__[], __modules_end__[];
}
void ModuleManager::parseInternal() {
  const size_t modules_size = size_t(__modules_end__ - __modules_start__);
  if (modules_size > 1) {
    loadMemory(__modules_start__, modules_size);
  }
}
void ModuleManager::parseInitRD() {
  Multiboot::Payload *multiboot = Multiboot::getPayload();
  if (!multiboot || (multiboot->flags & Multiboot::FLAG_MODS) == 0) return;
  const Multiboot::Module *mods =
      reinterpret_cast<const Multiboot::Module*>(uintptr_t(multiboot->mods.paddr));
  for (uint32_t i = 0; i < multiboot->mods.count; i++) {
    const uint8_t *base = reinterpret_cast<const uint8_t*>(uintptr_t(mods[i].start));
    const uint8_t *top = reinterpret_cast<const uint8_t*>(uintptr_t(mods[i].end));
    size_t length = size_t(top - base);
    loadMemory(base, length);
  }
}
void ModuleManager::init() {
  ModuleManager *mm = getManager();
  mm->parseInternal();
  mm->parseInitRD();
}

ModuleManager* ModuleManager::getManager() {
  if (manager) return const_cast<ModuleManager*>(manager);
  Mutex::Lock lock(managerMutex);
  if (!manager) manager = new ModuleManager();
  return const_cast<ModuleManager*>(manager);
}
