//    PhoeniX OS Process subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
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
  ptr<char> name;
  uintptr_t entry;
  List<Thread*> threads;
  Pagetable::Entry *pagetable;
  void *iomap[2];

  uintptr_t _aslrCode, _aslrStack;
  Pagetable::Entry* addPage(uintptr_t vaddr, void* paddr, Pagetable::MemoryType type);

  friend class KernelLinker;
  friend class Scheduler;

 public:
  Process();
  ~Process();

  uint64_t getId() const { return id; }

  uintptr_t addSection(uintptr_t offset, SectionType type, size_t size);
  void setEntryAddress(uintptr_t ptr);

  void allowIOPorts(uint16_t min, uint16_t max);

  void writeData(uintptr_t address, const void* src, size_t size);
  bool readData(void* dst, uintptr_t address, size_t size) const;
  char* readString(uintptr_t address) const;

  const char *getName() const __attribute__((pure));
  void setName(const char *name);
  uintptr_t getBase() const __attribute__((pure)) { return _aslrCode; }

  void* getPhysicalAddress(uintptr_t ptr) const __attribute__((pure));

  static size_t print_stacktrace(char *outbuf, size_t bufsz, uintptr_t base = 0, const Process *process = nullptr);
};
