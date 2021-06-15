//    PhoeniX OS Modules subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"
#include "process.hpp"
#include "kernlink.hpp"

class ModuleManager {
 private:
  struct ModuleInfo {
    ptr<char> name, version, description, requirements, developer;
  };
  static Mutex managerMutex;
  static volatile ModuleManager* manager;
  void parseInternal();
  void parseInitRD();
  void loadMemory(const void *mem, size_t size);
  bool parseModuleInfo(ModuleInfo *info, const Process *process, const KernelLinker *linker);
  bool bindRequirement(const char *req, Process *process);
  bool bindRequirements(const char *reqs, Process *process);
  static void init();

 public:
  static ModuleManager* getManager();
};
