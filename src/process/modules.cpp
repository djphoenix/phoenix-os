//    PhoeniX OS Modules subsystem
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

#include "modules.hpp"

#include "readelf.hpp"
#include "multiboot_info.hpp"

Mutex ModuleManager::managerMutex;

bool ModuleManager::parseModuleInfo(MODULEINFO *info, Process *process) {
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
  MODULEINFO mod = { 0, 0, 0, 0, 0 };

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

ModuleManager* ModuleManager::manager = 0;
void ModuleManager::loadStream(Stream *stream, bool start) {
  Stream *sub = stream;
  Process *process;
  size_t size;
  MODULEINFO mod;
  for (;;) {
    mod = {0, 0, 0, 0, 0};
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
    sub->seek(size, -1);
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
  asm("lea __modules_start__(%%rip), %q0":"=r"(mods_start));
  asm("lea __modules_end__(%%rip), %q0":"=r"(mods_end));

  size_t modules_size = mods_end - mods_start;

  if (modules_size > 1) {
    MemoryStream ms(mods_start, modules_size);
    loadStream(&ms, 1);
  }
}
void ModuleManager::parseInitRD() {
  MULTIBOOT_PAYLOAD *multiboot = Multiboot::getPayload();
  if (!multiboot || (multiboot->flags & MB_FLAG_MODS) == 0) return;
  const MULTIBOOT_MODULE *mods =
      reinterpret_cast<const MULTIBOOT_MODULE*>(multiboot->pmods_addr);
  for (uint32_t i = 0; i < multiboot->mods_count; i++) {
    const char *base = reinterpret_cast<const char*>(mods[i].start);
    const char *top = reinterpret_cast<const char*>(mods[i].end);
    size_t length = top - base;
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
ModuleManager::ModuleManager() {}
