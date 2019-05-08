//    PhoeniX OS Process subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

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
  SectionTypeCode,
  SectionTypeData,
  SectionTypeROData,
  SectionTypeBSS,
  SectionTypeStack
};

class Process {
 private:
  uint64_t id;
  List<Thread*> threads;
  List<ProcessSymbol> symbols;
  uintptr_t entry;
  void addPage(uintptr_t vaddr, void* paddr, uint8_t flags);
  uintptr_t _aslrCode, _aslrStack;

 public:
  Process();
  ~Process();
  void startup();
  void exit(int code);
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

  void* getPhysicalAddress(uintptr_t ptr) PURE;
};
