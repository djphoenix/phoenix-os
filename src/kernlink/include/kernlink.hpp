#pragma once

#include <stdint.h>
#include <stddef.h>
#include "list.hpp"

class Process;
class KernelLinker {
private:
  struct ProcessSymbol {
    uintptr_t ptr;
    char* name;
  };

  Process *const process;
  uintptr_t _syscallPage = 0;
  size_t _syscallNum = 0;
  List<ProcessSymbol> symbols {};
public:
  inline KernelLinker(Process *process) : process(process) {}
  ~KernelLinker();
  uintptr_t getSymbolByName(const char* name) const __attribute__((pure));
  void addSymbol(const char *name, uintptr_t ptr);
  uintptr_t linkLibrary(const char* funcname);
  void prepareToStart();
};
