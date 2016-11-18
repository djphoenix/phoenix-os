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
  ModuleManager();
  static ModuleManager* getManager();
};

