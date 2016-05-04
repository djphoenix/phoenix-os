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

#ifndef STREAM_H
#define STREAM_H
#include "pxlib.hpp"
#include "memory.hpp"

class Stream {
public:
	virtual _uint64 read(void* dest, _uint64 size)=0;
	virtual _uint64 size()=0;
	virtual _uint64 seek(_uint64 offset, char base = 0)=0;
	virtual Stream* substream(_uint64 offset=-1, _uint64 limit=-1)=0;
	virtual bool eof()=0;
	virtual char* readstr(_uint64 offset=-1)=0;
};

class MemoryStream:public Stream {
private:
	_uint64 offset;
	_uint64 limit;
	void* memory;
public:
	MemoryStream(void* memory, _uint64 limit);
	_uint64 read(void* dest, _uint64 size);
	_uint64 size();
	_uint64 seek(_uint64 offset, char base = 0);
	Stream* substream(_uint64 offset=-1, _uint64 limit=-1);
	char* readstr(_uint64 offset=-1);
	bool eof();
};

#endif