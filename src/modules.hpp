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
	void loadStream(Stream *stream, bool start=0);
	bool parseModuleInfo(PMODULEINFO mod, Stream *stream);
	PMODULEINFO loadElf(Stream *stream);
public:
	ModuleManager();
	static ModuleManager* getManager();
	static void init();
};

typedef struct {
	uint32_t name, type;
	uint64_t flags, addr, offset, size;
	uint32_t link, info;
	uint64_t addralign, entsize;
} ELF64SECT;
typedef struct{
	uint32_t name;
	uint8_t info, other;
	uint16_t shndx;
	uint64_t value;
	uint64_t size;
} ELF64SYM;
typedef struct{
	uint64_t addr;
	struct{ uint32_t type, sym;} info;
	uint64_t add;
} ELF64RELA;
typedef struct{
	uint64_t addr;
	struct{ uint32_t type, sym;} info;
} ELF64REL;
#endif
