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

#include "stream.hpp"

MemoryStream::MemoryStream(void* memory, size_t limit) {
  this->memory = static_cast<char*>(memory);
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
    return (this->offset = offset);
  if (base == 1)
    return (this->offset = this->limit - offset);
  return (this->offset += offset);
}
size_t MemoryStream::read(void* dest, size_t size) {
  if (offset + size >= limit)
    size = limit - offset;
  Memory::copy(dest, memory + offset, size);
  return size;
}
Stream* MemoryStream::substream(int64_t offset, size_t limit) {
  if (offset == -1)
    offset = this->offset;
  if ((limit == (size_t)-1) || (limit > this->limit - offset))
    limit = this->limit - offset;
  return new MemoryStream(memory + (size_t)offset, limit);
}
char* MemoryStream::readstr(int64_t offset) {
  if (offset == -1)
    offset = this->offset;
  if ((size_t)offset > limit)
    return 0;
  size_t len = strlen(memory + (size_t)offset, limit - offset);
  char* res = new char[len + 1]();
  Memory::copy(res, memory + (size_t)offset, len);
  res[len] = 0;
  return res;
}
