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

MemoryStream::MemoryStream(void* memory, _uint64 limit){
    this->memory = memory;
    this->limit = limit;
    this->offset = 0;
}
bool MemoryStream::eof(){
    return offset == limit;
}
_uint64 MemoryStream::size(){
    return limit-offset;
}
_uint64 MemoryStream::seek(_uint64 offset, char base){
    if(base==-1) return (this->offset=offset);
    if(base== 1) return (this->offset=this->limit-offset);
    return (this->offset+=offset);
}
_uint64 MemoryStream::read(void* dest, _uint64 size){
    if (offset+size >= limit) size = limit-offset;
    Memory::copy(dest,&((char*)memory)[offset],size);
    return size;
}
Stream* MemoryStream::substream(_uint64 offset, _uint64 limit){
    if (offset == (_uint64)-1) offset = this->offset;
    if ((limit == (_uint64)-1) || (limit > this->limit - offset)) limit = this->limit - offset;
    return new MemoryStream(&((char*)memory)[offset],limit);
}
char* MemoryStream::readstr(_uint64 offset){
    if (offset == (_uint64)-1) offset = this->offset;
    if (offset > limit) return 0;
    _uint64 len = strlen(&((char*)memory)[offset],limit-offset);
    char* res = (char*)Memory::alloc(len+1);
    Memory::copy(res,&((char*)memory)[offset],len);
    res[len]=0;
    return res;
}
