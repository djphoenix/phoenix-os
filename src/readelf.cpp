//    PhoeniX OS ELF binary reader
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

#include "readelf.hpp"

struct {
  struct {
    uint32_t magic;
    uint8_t eclass, data, version, osabi, abiversion, pad, rsvd[6];
  } ident;
  uint16_t type, machine;
  uint32_t version;
  uint64_t entry, phoff, shoff;
  uint32_t flags;
  uint16_t ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
}__attribute__((packed)) Elf;
typedef enum
  : uint32_t {
    SHT_NULL,
  SHT_PROGBITS,
  SHT_SYMTAB,
  SHT_STRTAB,
  SHT_RELA,
  SHT_HASH,
  SHT_DYNAMIC,
  SHT_NOTE,
  SHT_NOBITS,
  SHT_REL,
  SHT_SHLIB,
  SHT_DYNSYM
} ELF64_SECT_TYPE;
typedef enum
  : uint64_t {
    SHF_WRITE = 1, SHF_ALLOC = 2, SHF_EXECINSTR = 4
} ELF64_SECT_FLAGS;
typedef struct {
  uint32_t name;
  ELF64_SECT_TYPE type;
  uint64_t flags, addr, offset, size;
  uint32_t link, info;
  uint64_t addralign, entsize;
}__attribute__((packed)) ELF64SECT;
typedef struct {
  uint32_t name;
  uint8_t info, other;
  uint16_t shndx;
  uint64_t value;
  uint64_t size;
}__attribute__((packed)) ELF64SYM;
typedef struct {
  uint64_t addr;
  struct {
    uint32_t type, sym;
  } info;
  uint64_t add;
}__attribute__((packed)) ELF64RELA;
typedef struct {
  uint64_t addr;
  struct {
    uint32_t type, sym;
  } info;
}__attribute__((packed)) ELF64REL;

size_t readelf(Process *process, Stream *stream) {
  if ((stream->read(&Elf, sizeof(Elf)) != sizeof(Elf)) || (Elf.ident.magic
      != 'FLE\x7F')
      || (Elf.ident.eclass != 2) || (Elf.ident.data != 1)
      || (Elf.ident.version != 1) || (Elf.ident.osabi != 0)
      || (Elf.ident.abiversion != 0) || (Elf.type != 1) || (Elf.version != 1)
      || (Elf.ehsize != sizeof(Elf))
    ) return 0;
  size_t size = MAX(Elf.phoff + Elf.phentsize * Elf.phnum,
                    Elf.shoff + Elf.shentsize * Elf.shnum);
  ELF64SECT *sections = (ELF64SECT*)Memory::alloc(
      sizeof(ELF64SECT) * Elf.shnum);
  stream->seek(Elf.shoff);
  stream->read(sections, sizeof(ELF64SECT) * Elf.shnum);
  char *names = 0;
  if (Elf.shstrndx != 0) {
    ELF64SECT sect = sections[Elf.shstrndx];
    names = (char*)Memory::alloc(sect.size);
    stream->seek(sect.offset, -1);
    stream->read(names, sect.size);
  }
  uintptr_t *sectmap = (uintptr_t*)Memory::alloc(sizeof(uintptr_t) * Elf.shnum);
  // Fill up process sections
  for (uint32_t i = 0; i < Elf.shnum; i++) {
    ELF64SECT sect = sections[i];
    size = MAX(size, sect.offset + sect.size);
    if (sect.type == SHT_NULL)
      continue;
    if (sect.type == SHT_SYMTAB) {
      ELF64SYM *symbols = (ELF64SYM*)Memory::alloc(sect.size);
      stream->seek(sect.offset, -1);
      stream->read(symbols, sect.size);
      uint32_t symcount = sect.size / sizeof(ELF64SYM);
      char *symnames = 0;
      if (sect.link != 0) {
        ELF64SECT nsect = sections[sect.link];
        symnames = (char*)Memory::alloc(nsect.size);
        stream->seek(nsect.offset, -1);
        stream->read(symnames, nsect.size);
      }
      for (uint32_t i = 0; i < symcount; i++) {
        ELF64SYM sym = symbols[i];
        char *name = (symnames != 0 ? &symnames[sym.name] : 0);
        if (name == 0 || name[0] == 0)
          continue;
        ELF64SECT sect = sections[sym.shndx];
        if (sect.type == SHT_NULL)
          continue;
        uintptr_t ps = sectmap[sym.shndx];
        if (ps == 0)
          continue;
        uintptr_t offset = ps + sym.value;
        process->addSymbol(name, offset);
      }
      Memory::free(symbols);
      Memory::free(symnames);
    }
    if ((sect.flags & SHF_ALLOC) == 0)
      continue;
    SectionType type = SectionTypeData;
    if ((sect.flags & SHF_EXECINSTR) != 0)
      type = SectionTypeCode;
    else if (sect.type == SHT_NOBITS)
      type = SectionTypeBSS;
    uintptr_t ps = process->addSection(type, sect.size);
    sectmap[i] = ps;
    if (sect.type != SHT_NOBITS) {
      stream->seek(sect.offset, -1);
      void *buf = Memory::alloc(sect.size);
      stream->read(buf, sect.size);
      process->writeData(ps, buf, sect.size);
      Memory::free(buf);
    }
  }
  // Process relocs
  for (uint32_t i = 0; i < Elf.shnum; i++) {
    ELF64SECT relsect = sections[i];
    if (relsect.type != SHT_RELA && relsect.type != SHT_REL)
      continue;
    ELF64SECT symsect = sections[relsect.link];
    uintptr_t sectstart = sectmap[relsect.info];
    if (sectstart == 0)
      continue;
    if (symsect.type != SHT_SYMTAB)
      continue;
    size_t relcnt;
    ELF64RELA *relocs;
    stream->seek(relsect.offset, -1);
    if (relsect.type == SHT_RELA) {
      relcnt = relsect.size / sizeof(ELF64RELA);
      relocs = (ELF64RELA*)Memory::alloc(relsect.size);
      stream->read(relocs, relsect.size);
    } else {
      relcnt = relsect.size / sizeof(ELF64REL);
      relocs = (ELF64RELA*)Memory::alloc(sizeof(ELF64RELA) * relcnt);
      ELF64REL *_relocs = (ELF64REL*)Memory::alloc(relsect.size);
      stream->read(_relocs, relsect.size);
      for (size_t r = 0; r < relcnt; r++)
        relocs[r] = {
          _relocs[r].addr,
          { _relocs[r].info.type, _relocs[r].info.sym},
          0
        };
      Memory::free(_relocs);
    }

    ELF64SYM *symbols = (ELF64SYM*)Memory::alloc(symsect.size);
    stream->seek(symsect.offset, -1);
    stream->read(symbols, symsect.size);
    char *symnames = 0;
    if (symsect.link != 0) {
      ELF64SECT nsect = sections[symsect.link];
      symnames = (char*)Memory::alloc(nsect.size);
      stream->seek(nsect.offset, -1);
      stream->read(symnames, nsect.size);
    }

    for (size_t r = 0; r < relcnt; r++) {
      ELF64RELA rel = relocs[r];
      ELF64SYM sym = symbols[rel.info.sym];
      ELF64SECT _symsect = sections[sym.shndx];

      char *symname;
      uintptr_t addr, offset;
      if (sym.info == 3) {
        symname = names != 0 ? &names[_symsect.name] : 0;
        offset = sectmap[sym.shndx];
      } else {
        symname = (symnames != 0 ? &symnames[sym.name] : 0);
        offset = sym.value;
      }

      if (symsect.type == SHT_NULL) {
        printf("(link) %s\n", symname);
      }

      addr = sectstart + rel.addr;
      offset += rel.add;

      switch (rel.info.type) {
        case 0:   // R_X86_64_NONE
          break;
        case 1:   // R_X86_64_64
          process->writeData(addr, &offset, sizeof(uintptr_t));
          break;
        case 2:   // R_X86_64_PC32
        case 3:   // R_X86_64_GOT32
        case 4:   // R_X86_64_PLT32
        case 5:   // R_X86_64_COPY
        case 6:   // R_X86_64_GLOB_DAT
        case 7:   // R_X86_64_JUMP_SLOT
        case 8:   // R_X86_64_RELATIVE
        case 9:   // R_X86_64_GOTPCREL
        case 10:  // R_X86_64_32
        case 11:  // R_X86_64_32S
        case 12:  // R_X86_64_16
        case 13:  // R_X86_64_PC16
        case 14:  // R_X86_64_8
        case 15:  // R_X86_64_PC8
        case 16:  // R_X86_64_NUM
        default:
          printf("Unhandled reloc type: %x\n", rel.info.type);
          break;
      }
    }
    Memory::free(relocs);
    Memory::free(symbols);
    Memory::free(symnames);
  }
  Memory::free(sectmap);
  Memory::free(sections);
  Memory::free(names);
  return size;
}
