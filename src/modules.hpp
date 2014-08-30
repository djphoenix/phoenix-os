//    PhoeniX OS Modules subsystem
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

#ifndef MODULES_H
#define MODULES_H
#include "pxlib.hpp"
#include "multiboot_info.hpp"
#include "process.hpp"
#include "stream.hpp"

typedef struct{
	char *name, *version, *description, *requirements, *developer;
	_uint64 size;
    PROCSTARTINFO psinfo;
} MODULEINFO, *PMODULEINFO;
class ModuleManager {
private:
    static ModuleManager* manager;
    void parseInternal();
    void parseInitRD();
    void loadStream(Stream *stream);
    bool parseModuleInfo(PMODULEINFO mod, Stream *stream);
    PMODULEINFO loadElf(Stream *stream);
public:
    ModuleManager();
    static ModuleManager* getManager();
    static void init();
};

typedef struct {
    uint name, type;
    _uint64 flags, addr, offset, size;
    uint link, info;
    _uint64 addralign, entsize;
} ELF64SECT;
typedef struct{
	uint name;
	unsigned char info, other;
    unsigned short shndx;
    _uint64 value;
    _uint64 size;
} ELF64SYM;
typedef struct{
	_uint64 addr;
    struct{ uint type, sym;} info;
    _uint64 add;
} ELF64RELA;
typedef struct{
	_uint64 addr;
    struct{ uint type, sym;} info;
} ELF64REL;
#endif
