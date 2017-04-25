//    PhoeniX OS Modules subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"
#include "process.hpp"
#include "stream.hpp"

struct MODULEINFO {
  char *name, *version, *description, *requirements, *developer;
};
class ModuleManager {
 private:
  static Mutex managerMutex;
  static ModuleManager* manager;
  void parseInternal();
  void parseInitRD();
  void loadStream(Stream *stream, bool start = 0);
  bool parseModuleInfo(MODULEINFO *info, Process *process);
  static void init();

 public:
  static ModuleManager* getManager();
};

