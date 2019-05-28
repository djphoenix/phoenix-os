//    PhoeniX OS Modules subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "modules.hpp"

#include "readelf.hpp"
#include "multiboot_info.hpp"

volatile ModuleManager* ModuleManager::manager = nullptr;
Mutex ModuleManager::managerMutex;

bool ModuleManager::parseModuleInfo(ModuleInfo *info, const Process *process) {
  struct {
    uintptr_t name, version, desc, reqs, dev;
  } symbols = {
    process->getSymbolByName("module_name"),
    process->getSymbolByName("module_version"),
    process->getSymbolByName("module_description"),
    process->getSymbolByName("module_requirements"),
    process->getSymbolByName("module_developer")
  };
  ModuleInfo mod = { nullptr, nullptr, nullptr, nullptr, nullptr };

  if ((symbols.name == 0) || (symbols.version == 0)
      || (symbols.desc == 0) || (symbols.reqs == 0) || (symbols.dev == 0))
    return false;

  mod.name = process->readString(symbols.name);
  mod.version = process->readString(symbols.version);
  mod.description = process->readString(symbols.desc);
  mod.requirements = process->readString(symbols.reqs);
  mod.developer = process->readString(symbols.dev);

  *info = mod;

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
  char *r;
  while (*reqs != 0) {
    if (*reqs == ';') { reqs++; continue; }
    re = reqs;
    while (*re != 0 && *re != ';') re++;
    r = klib::strndup(reqs, size_t(re - reqs));
    reqs = re;

    if (!bindRequirement(r, process)) {
      printf("Unsatisfied requirement: %s\n", r);
      Heap::free(r);
      return 0;
    }
    Heap::free(r);
  }
  return 1;
}

void ModuleManager::loadStream(Stream *stream) {
  Stream *sub = stream;
  Process *process;
  size_t size;
  ModuleInfo mod;
  for (;;) {
    mod = {nullptr, nullptr, nullptr, nullptr, nullptr};
    process = new Process();
    size = readelf(process, sub);
    if (size == 0 || !parseModuleInfo(&mod, process)) {
      delete process;
      break;
    }
    process->setName(mod.name);
    if (bindRequirements(mod.requirements, process)) {
      process->startup();
    } else {
      delete process;
    }
    delete mod.name;
    delete mod.version;
    delete mod.description;
    delete mod.requirements;
    delete mod.developer;

    sub->seek(ptrdiff_t(size), -1);
    if (!stream->eof()) {
      Stream *_sub = sub->substream();
      if (sub != stream) delete sub;
      sub = _sub;
    }
  }
  if (sub != stream)
    delete sub;
}
void ModuleManager::parseInternal() {
  const char *mods_start, *mods_end;
  asm volatile("lea __modules_start__(%%rip), %q0":"=r"(mods_start));
  asm volatile("lea __modules_end__(%%rip), %q0":"=r"(mods_end));

  size_t modules_size = size_t(mods_end - mods_start);

  if (modules_size > 1) {
    MemoryStream ms(mods_start, modules_size);
    loadStream(&ms);
  }
}
void ModuleManager::parseInitRD() {
  Multiboot::Payload *multiboot = Multiboot::getPayload();
  if (!multiboot || (multiboot->flags & Multiboot::FLAG_MODS) == 0) return;
  const Multiboot::Module *mods =
      reinterpret_cast<const Multiboot::Module*>(uintptr_t(multiboot->mods.paddr));
  for (uint32_t i = 0; i < multiboot->mods.count; i++) {
    const char *base = reinterpret_cast<const char*>(uintptr_t(mods[i].start));
    const char *top = reinterpret_cast<const char*>(uintptr_t(mods[i].end));
    size_t length = size_t(top - base);
    MemoryStream ms(base, length);
    loadStream(&ms);
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
