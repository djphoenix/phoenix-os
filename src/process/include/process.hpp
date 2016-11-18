//    PhoeniX OS Process subsystem
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
#include "thread.hpp"
#include "pagetable.hpp"
#include "list.hpp"


struct ProcessSymbol {
  uintptr_t ptr;
  char* name;
};

enum SectionType: uint8_t {
    SectionTypeCode, SectionTypeData, SectionTypeBSS, SectionTypeStack,
};

class Process {
 private:
  uint64_t id;
  List<Thread*> threads;
  List<ProcessSymbol> symbols;
  uintptr_t entry;
  void addPage(uintptr_t vaddr, void* paddr, uint8_t flags);

 public:
  Process();
  ~Process();
  void startup();
  void addThread(Thread *thread, bool suspended);

  uint64_t getId() { return id; }

  PTE *pagetable;
  uintptr_t addSection(SectionType type, size_t size);
  void addSymbol(const char *name, uintptr_t ptr);
  void setEntryAddress(uintptr_t ptr);

  uintptr_t getSymbolByName(const char* name) PURE;
  uintptr_t linkLibrary(const char* funcname);

  void writeData(uintptr_t address, void* src, size_t size);
  void readData(void* dst, uintptr_t address, size_t size);
  char* readString(uintptr_t address);

  uintptr_t getVirtualAddress(void* addr) CONST;
  void* getPhysicalAddress(uintptr_t ptr) PURE;
};
