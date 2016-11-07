//    PhoeniX OS Stream Subsystem
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
#include "pxlib.hpp"
#include "heap.hpp"

class Stream {
 public:
  virtual size_t read(void* dest, size_t size) = 0;
  virtual size_t size() = 0;
  virtual size_t seek(int64_t offset, char base = 0) = 0;
  virtual Stream* substream(int64_t offset = -1, size_t limit = -1) = 0;
  virtual bool eof() = 0;
  virtual char* readstr(int64_t offset = -1) = 0;
  virtual ~Stream() {}
};

class MemoryStream: public Stream {
 private:
  size_t offset;
  size_t limit;
  char* memory;

 public:
  MemoryStream(void* memory, size_t limit);
  size_t read(void* dest, size_t size);
  size_t size();
  size_t seek(int64_t offset, char base = 0);
  Stream* substream(int64_t offset = -1, size_t limit = -1);
  char* readstr(int64_t offset = -1);
  bool eof();
};
