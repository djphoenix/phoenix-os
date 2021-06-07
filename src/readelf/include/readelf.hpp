//    PhoeniX OS ELF binary reader
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"
#include "process.hpp"

size_t readelf(Process *process, const void *mem, size_t size);
