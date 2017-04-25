//    PhoeniX OS ELF binary reader
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"
#include "process.hpp"
#include "stream.hpp"

size_t readelf(Process *process, Stream *stream);
