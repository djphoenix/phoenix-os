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
extern void modules_init();
typedef struct {
	struct {
		uint EI_MAG;
		char EI_CLASS;
		char EI_DATA;
		char EI_VERSION;
		char EI_OSABI;
		char EI_ABIVERSION;
		char EI_PAD;
		char reserved[6];
	} e_ident;
	unsigned short e_type; /* Object file type */
	unsigned short e_machine; /* Machine type */
	uint e_version; /* Object file version */
	void* e_entry; /* Entry point address */
	_uint64 e_phoff; /* Program header offset */
	_uint64 e_shoff; /* Section header offset */
	uint e_flags; /* Processor-specific flags */
	unsigned short e_ehsize; /* ELF header size */
	unsigned short e_phentsize; /* Size of program header entry */
	unsigned short e_phnum; /* Number of program header entries */
	unsigned short e_shentsize; /* Size of section header entry */
	unsigned short e_shnum; /* Number of section header entries */
	unsigned short e_shstrndx; /* Section name string table index */
} ELF64HDR, *PELF64HDR;
typedef struct
{
	uint sh_name;
	uint sh_type;
	_uint64 sh_flags;
	_uint64 sh_addr;
	_uint64 sh_offset;
	_uint64 sh_size;
	uint sh_link;
	uint sh_info;
	_uint64 sh_addralign;
	_uint64 sh_entsize;
} ELF64SECT, *PELF64SECT;
typedef struct{
	uint st_name;
	unsigned char st_info, st_other;
    unsigned short st_shndx;
    void* st_value;
    _uint64 st_size;
} ELF64SYM, *PELF64SYM;
typedef struct{
	char *name, *version, *description, *requirements, *developer;
	_uint64 size;
    PROCSTARTINFO psinfo;
} MODULEINFO, *PMODULEINFO;
typedef struct{
    uint type;
    uint sym;
} ELFRELI;
typedef struct{
	_uint64 addr;
    ELFRELI info;
    _uint64 add;
} ELF64RELA, *PELF64RELA;
typedef struct{
	_uint64 addr;
    ELFRELI info;
} ELF64REL, *PELF64REL;
#endif
