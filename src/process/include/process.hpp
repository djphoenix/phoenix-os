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
  Pagetable::Entry* addPage(uintptr_t vaddr, void* paddr, uint8_t flags);
  uintptr_t _aslrCode, _aslrStack, _syscallPage;
  size_t _syscallNum;
  void *iomap[2];
  char *name;

 public:
  Process();
  ~Process();
  void startup();
  void exit(int code);
  void addThread(Thread *thread, bool suspended);

  uint64_t getId() { return id; }

  Pagetable::Entry *pagetable;
  uintptr_t addSection(SectionType type, size_t size);
  void addSymbol(const char *name, uintptr_t ptr);
  void setEntryAddress(uintptr_t ptr);

  uintptr_t getSymbolByName(const char* name) PURE;
  uintptr_t linkLibrary(const char* funcname);
  void allowIOPorts(uint16_t min, uint16_t max);

  void writeData(uintptr_t address, const void* src, size_t size);
  void readData(void* dst, uintptr_t address, size_t size) const;
  char* readString(uintptr_t address) const;

  const char *getName() const;
  void setName(const char *name);

  void* getPhysicalAddress(uintptr_t ptr) const PURE;

  static void print_stacktrace(uintptr_t base = 0, const Process *process = nullptr);
};
