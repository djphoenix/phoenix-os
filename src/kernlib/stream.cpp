//    PhoeniX OS Stream Subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "stream.hpp"

MemoryStream::MemoryStream(const void* memory, size_t limit) {
  this->memory = static_cast<const char*>(memory);
  this->limit = limit;
  this->offset = 0;
}
bool MemoryStream::eof() {
  return offset == limit;
}
size_t MemoryStream::size() {
  return limit - offset;
}
size_t MemoryStream::seek(int64_t offset, char base) {
  if (base == -1)
    return (this->offset = size_t(offset));
  if (base == 1)
    return (this->offset = size_t(ptrdiff_t(this->limit) - offset));
  return (this->offset = size_t(ptrdiff_t(this->offset) + offset));
}
size_t MemoryStream::read(void* dest, size_t size) {
  if (offset + size >= limit)
    size = limit - offset;
  Memory::copy(dest, memory + offset, size);
  offset += size;
  return size;
}
Stream* MemoryStream::substream(int64_t offset, size_t limit) {
  if (offset == -1)
    offset = ptrdiff_t(this->offset);
  if ((limit == size_t(-1)) || (limit > size_t(ptrdiff_t(this->limit) - offset)))
    limit = size_t(ptrdiff_t(this->limit) - offset);
  return new MemoryStream(memory + size_t(offset), limit);
}
char* MemoryStream::readstr() {
  size_t len = klib::strlen(memory + offset, limit - offset);
  char* res = new char[len + 1]();
  Memory::copy(res, memory + offset, len);
  res[len] = 0;
  return res;
}
