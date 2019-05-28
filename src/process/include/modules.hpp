//    PhoeniX OS Modules subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"
#include "process.hpp"
#include "stream.hpp"

class ModuleManager {
 private:
  struct ModuleInfo {
    ptr<char> name, version, description, requirements, developer;
  };
  static Mutex managerMutex;
  static volatile ModuleManager* manager;
  void parseInternal();
  void parseInitRD();
  void loadStream(Stream *stream);
  bool parseModuleInfo(ModuleInfo *info, const Process *process);
  bool bindRequirement(const char *req, Process *process);
  bool bindRequirements(const char *reqs, Process *process);
  static void init();

 public:
  static ModuleManager* getManager();
};

