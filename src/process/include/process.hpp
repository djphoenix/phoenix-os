//    PhoeniX OS Process subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"
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

struct Thread;
class Process {
 private:
  uint64_t id;
  List<Thread*> threads;
  uintptr_t entry;
  Pagetable::Entry* addPage(uintptr_t vaddr, void* paddr, Pagetable::MemoryType type);
  uintptr_t _aslrCode, _aslrStack;
  void *iomap[2];
  ptr<char> name;
  friend class KernelLinker;

 public:
  Process();
  ~Process();
  void startup();
  void exit(int code);
  void addThread(Thread *thread, bool suspended);

  uint64_t getId() const { return id; }

  Pagetable::Entry *pagetable;
  uintptr_t addSection(uintptr_t offset, SectionType type, size_t size);
  void setEntryAddress(uintptr_t ptr);

  void allowIOPorts(uint16_t min, uint16_t max);

  void writeData(uintptr_t address, const void* src, size_t size);
  bool readData(void* dst, uintptr_t address, size_t size) const;
  char* readString(uintptr_t address) const;

  const char *getName() const PURE;
  void setName(const char *name);
  uintptr_t getBase() const PURE { return _aslrCode; }

  void* getPhysicalAddress(uintptr_t ptr) const PURE;

  static size_t print_stacktrace(char *outbuf, size_t bufsz, uintptr_t base = 0, const Process *process = nullptr);
};
