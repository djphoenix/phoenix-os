//    PhoeniX OS Stream Subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"

class Stream {
 public:
  virtual size_t read(void* dest, size_t size) = 0;
  virtual size_t size() = 0;
  virtual size_t seek(int64_t offset, char base = 0) = 0;
  virtual Stream* substream(int64_t offset = -1, size_t limit = -1) = 0;
  virtual bool eof() = 0;
  virtual char* readstr() = 0;
  virtual ~Stream() {}
};

class MemoryStream: public Stream {
 private:
  size_t offset;
  size_t limit;
  const char* memory;

 public:
  MemoryStream(const void* memory, size_t limit);
  size_t read(void* dest, size_t size);
  size_t size() PURE;
  size_t seek(int64_t offset, char base = 0);
  Stream* substream(int64_t offset = -1, size_t limit = -1);
  char* readstr();
  bool eof() PURE;
};
