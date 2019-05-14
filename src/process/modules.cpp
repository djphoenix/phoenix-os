//    PhoeniX OS Modules subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "modules.hpp"

#include "readelf.hpp"
#include "multiboot_info.hpp"

Mutex ModuleManager::managerMutex;

bool ModuleManager::parseModuleInfo(ModuleInfo *info, Process *process) {
  struct {
    uintptr_t entry, name, version, desc, reqs, dev;
  } symbols = {
    process->getSymbolByName("module"),
    process->getSymbolByName("module_name"),
    process->getSymbolByName("module_version"),
    process->getSymbolByName("module_description"),
    process->getSymbolByName("module_requirements"),
    process->getSymbolByName("module_developer")
  };
  ModuleInfo mod = { nullptr, nullptr, nullptr, nullptr, nullptr };

  if ((symbols.entry == 0) || (symbols.name == 0) || (symbols.version == 0)
      || (symbols.desc == 0) || (symbols.reqs == 0) || (symbols.dev == 0))
    return false;

  process->setEntryAddress(symbols.entry);

  mod.name = process->readString(symbols.name);
  mod.version = process->readString(symbols.version);
  mod.description = process->readString(symbols.desc);
  mod.requirements = process->readString(symbols.reqs);
  mod.developer = process->readString(symbols.dev);

  *info = mod;

  return true;
}

ModuleManager* ModuleManager::manager = nullptr;
void ModuleManager::loadStream(Stream *stream, bool start) {
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
    delete mod.name;
    delete mod.version;
    delete mod.description;
    delete mod.requirements;
    delete mod.developer;
    if (start) process->startup();
    sub->seek(ptrdiff_t(size), -1);
    if (!stream->eof()) {
      Stream *_sub = sub->substream();
      if (sub != stream)
        delete sub;
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
    loadStream(&ms, 1);
  }
}
void ModuleManager::parseInitRD() {
  Multiboot::Payload *multiboot = Multiboot::getPayload();
  if (!multiboot || (multiboot->flags & Multiboot::MB_FLAG_MODS) == 0) return;
  const Multiboot::Module *mods =
      reinterpret_cast<const Multiboot::Module*>(uintptr_t(multiboot->pmods_addr));
  for (uint32_t i = 0; i < multiboot->mods_count; i++) {
    const char *base = reinterpret_cast<const char*>(uintptr_t(mods[i].start));
    const char *top = reinterpret_cast<const char*>(uintptr_t(mods[i].end));
    size_t length = size_t(top - base);
    MemoryStream ms(base, length);
    loadStream(&ms, 1);
  }
}
void ModuleManager::init() {
  ModuleManager *mm = getManager();
  mm->parseInternal();
  mm->parseInitRD();
}

ModuleManager* ModuleManager::getManager() {
  if (manager) return manager;
  managerMutex.lock();
  if (!manager)
    manager = new ModuleManager();
  managerMutex.release();
  return manager;
}
