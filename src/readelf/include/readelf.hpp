//    PhoeniX OS ELF binary reader
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "process.hpp"
#include "kernlink.hpp"

size_t readelf(Process *process, KernelLinker *linker, const void *mem, size_t size);
