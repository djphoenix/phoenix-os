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
  size_t size;
  MODULEINFO mod;
  parse: mod = {0, 0, 0, 0, 0};
  Process *process = new Process();
  size = readelf(process, sub);
  if (size == 0) {
    delete process;
    goto end;
  }
  if (!parseModuleInfo(&mod, process)) {
    delete process;
    goto end;
  }
  if (mod.name) delete[] mod.name;
  if (mod.version) delete[] mod.version;
  if (mod.description) delete[] mod.description;
  if (mod.requirements) delete[] mod.requirements;
  if (mod.developer) delete[] mod.developer;
  if (start)
    process->startup();
  sub->seek(size, -1);
  if (!stream->eof()) {
    Stream *_sub = sub->substream();
    if (sub != stream)
      delete sub;
    sub = _sub;
    goto parse;
  }
end:
  if (sub != stream)
    delete sub;
}
void ModuleManager::parseInternal() {
  extern char __modules_start__, __modules_end__;

  size_t modules_size = &__modules_end__ - &__modules_start__;

  if (modules_size > 1) {
    Stream *ms = new MemoryStream(&__modules_start__, modules_size);
    loadStream(ms, 1);
    delete ms;
  }
}
void ModuleManager::parseInitRD() {
  if (kernel_data.mods != 0) {
    MODULE *mod = kernel_data.mods;
    while (mod != 0) {
      Stream *ms = new MemoryStream(
          mod->start,
          (static_cast<char*>(mod->end) - static_cast<char*>(mod->start)));
      loadStream(ms, 1);
      mod = mod->next;
      delete ms;
    }
  }
}
void ModuleManager::init() {
  ModuleManager *mm = getManager();
  mm->parseInternal();
  mm->parseInitRD();
}
static Mutex moduleManagerMutex;
ModuleManager* ModuleManager::getManager() {
  if (manager) return manager;
  moduleManagerMutex.lock();
  if (!manager)
    manager = new ModuleManager();
  moduleManagerMutex.release();
  return manager;
}
ModuleManager::ModuleManager() {}
