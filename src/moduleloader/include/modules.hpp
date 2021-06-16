//    PhoeniX OS Modules subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once

#include "heap.hpp"

class Process;
class KernelLinker;
class ModuleManager {
 private:
  struct ModuleInfo {
    ptr<char> name, version, description, requirements, developer;
  };
  static ModuleManager manager;
  void parseInternal();
  void parseInitRD();
  void loadMemory(const void *mem, size_t size);
  bool parseModuleInfo(ModuleInfo *info, const Process *process, const KernelLinker *linker);
  bool bindRequirement(const char *req, Process *process);
  bool bindRequirements(const char *reqs, Process *process);
  static void init();

 public:
  static inline ModuleManager* getManager() { return &manager; }
};
