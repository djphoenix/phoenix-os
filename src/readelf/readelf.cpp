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

struct ELF_HDR {
  struct {
    uint32_t magic;
    uint8_t eclass, data, version, osabi, abiversion, pad, rsvd[6];
  } ident;
  uint16_t type, machine;
  uint32_t version;
  uint64_t entry, phoff, shoff;
  uint32_t flags;
  uint16_t ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
} PACKED;
enum ELF64_SECT_TYPE: uint32_t {
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
};
enum ELF64_SECT_FLAGS: uint64_t {
    SHF_WRITE = 1, SHF_ALLOC = 2, SHF_EXECINSTR = 4
};
struct ELF64SECT {
  uint32_t name;
  ELF64_SECT_TYPE type;
  uint64_t flags, addr, offset, size;
  uint32_t link, info;
  uint64_t addralign, entsize;
} PACKED;
struct ELF64SYM {
  uint32_t name;
  uint8_t info, other;
  uint16_t shndx;
  uint64_t value;
  uint64_t size;
} PACKED;
struct ELF64RELA {
  uint64_t addr;
  struct {
    uint32_t type, sym;
  } info;
  uint64_t add;
} PACKED;
struct ELF64REL {
  uint64_t addr;
  struct {
    uint32_t type, sym;
  } info;
} PACKED;

size_t readelf(Process *process, Stream *stream) {
  ELF_HDR *elf = new ELF_HDR();
  size_t size = 0;
  ELF64SECT *sections = 0;
  uintptr_t *sectmap = 0;
  char *buf = 0;
  ELF64SYM *symbols = 0;
  ELF64RELA *relocs = 0;
  ELF64REL *relocs_buf = 0;

  ELF64SECT *sectnamesect = 0;

  // Read and check header
  if ((stream->read(elf, sizeof(ELF_HDR)) != sizeof(ELF_HDR))
      || (elf->ident.magic != 0x464C457F)  // '\x7FELF'
      || (elf->ident.eclass != 2) || (elf->ident.data != 1)
      || (elf->ident.version != 1) || (elf->ident.osabi != 0)
      || (elf->ident.abiversion != 0)
      || (elf->type != 1) || (elf->version != 1)
      || (elf->ehsize != sizeof(ELF_HDR))) goto err;

  // Init variables
  size = MAX(elf->phoff + elf->phentsize * elf->phnum,
             elf->shoff + elf->shentsize * elf->shnum);
  sections = new ELF64SECT[elf->shnum]();
  sectmap = new uintptr_t[elf->shnum]();

  // Read section headers
  stream->seek(elf->shoff);
  if (stream->read(sections, sizeof(ELF64SECT) * elf->shnum)
      != sizeof(ELF64SECT) * elf->shnum)
    goto err;

  // Read section names
  sectnamesect = sections + elf->shstrndx;

  // Read section contents
  for (uint32_t s = 0; s < elf->shnum; s++) {
    ELF64SECT *sect = sections + s;
    // Check elf size
    if (sect->offset + sect->size > size)
      size = sect->offset + sect->size;
    // Skip null sections
    if (sect->type == SHT_NULL) continue;
    if ((sect->flags & SHF_ALLOC) == 0) continue;
    // Skip SYMTAB sections
    if (sect->type == SHT_SYMTAB) continue;
    // Skip empty sections
    if (sect->flags & SHT_NOBITS) continue;
    if (sect->size == 0) continue;
    // Section type
    SectionType type = SectionTypeData;
    if ((sect->flags & SHF_EXECINSTR) != 0)
      type = SectionTypeCode;
    else if (sect->type == SHT_NOBITS)
      type = SectionTypeBSS;
    // Add section
    uintptr_t vaddr = process->addSection(type, sect->size);
    sectmap[s] = vaddr;
    if (sect->type != SHT_NOBITS) {
      // Copy section data
      stream->seek(sect->offset, -1);
      buf = new char[sect->size]();
      if (stream->read(buf, sect->size) != sect->size)
        goto err;
      process->writeData(vaddr, buf, sect->size);
      delete buf; buf = 0;
    }
  }

  // Fill symbol table
  for (uint32_t s = 0; s < elf->shnum; s++) {
    ELF64SECT *sect = sections + s;
    // Skip non-symtab sections
    if (sect->type != SHT_SYMTAB) continue;
    // Calculate symbol count
    uint32_t symcount = sect->size / sizeof(ELF64SYM);
    // Read symbol table
    symbols = new ELF64SYM[symcount];
    stream->seek(sect->offset, -1);
    if (stream->read(symbols, sect->size) != sect->size) goto err;
    // Read symbol name table
    ELF64SECT *namesect = sections + sect->link;
    // Enumerate symbols
    for (uint32_t i = 0; i < symcount; i++) {
      ELF64SYM *sym = symbols + i;
      stream->seek(namesect->offset + sym->name, -1);
      char *name = namesect ? stream->readstr() : 0;
      // Skip unnamed symbols
      if (name == 0 || name[0] == 0) continue;
      // Skip symbols in undefined sections
      if (sym->shndx > elf->shnum) continue;
      // Find symbol section
      ELF64SECT *symsect = sections + sym->shndx;
      // Skip undefined symbols
      if (symsect->type == SHT_NULL) continue;
      // Skip non-existing sections
      if (sym->shndx >= elf->shnum) continue;
      // Find vaddr of symbol section
      uintptr_t symbase = sectmap[sym->shndx];
      // Skip non-allocated symbols
      if (symbase == 0) continue;
      // Add symbol to internal table
      uintptr_t offset = symbase + sym->value;
      process->addSymbol(name, offset);
    }
    // Free buffers
    delete symbols; symbols = 0;
  }

  // Process relocations
  for (uint32_t rs = 0; rs < elf->shnum; rs++) {
    ELF64SECT *relsect = sections + rs;
    // Skip non-reloc sections
    if (relsect->type != SHT_RELA && relsect->type != SHT_REL)
      continue;
    // Find symbol section
    ELF64SECT *symsect = sections + relsect->link;
    // Find section base
    uintptr_t sectbase = sectmap[relsect->info];
    // Skip non-allocated sections
    if (sectbase == 0) continue;
    // Skip invalid symbol sections
    if (symsect->type != SHT_SYMTAB) continue;
    // Calculate relocation count
    size_t relcnt = 0;
    if (relsect->type == SHT_RELA)
      relcnt = relsect->size / sizeof(ELF64RELA);
    if (relsect->type == SHT_REL)
      relcnt = relsect->size / sizeof(ELF64REL);
    // Skip empty reloc sections
    if (relcnt == 0) continue;
    // Alloc reloc buffer
    relocs = new ELF64RELA[relcnt]();
    // Read reloc list
    stream->seek(relsect->offset, -1);
    if (relsect->type == SHT_RELA) {
      if (stream->read(relocs, relsect->size) != relsect->size)
        goto err;
    } else {
      // Convert ELF64REL to ELF64RELA
      relocs_buf = new ELF64REL[relcnt]();
      if (stream->read(relocs_buf, relsect->size) != relsect->size)
        goto err;
      for (size_t r = 0; r < relcnt; r++)
        relocs[r] = {
                     relocs_buf[r].addr,
                     { relocs_buf[r].info.type, relocs_buf[r].info.sym },
                     0
        };
      delete relocs_buf; relocs_buf = 0;
    }
    // Read symbol table
    size_t symcnt = symsect->size / sizeof(ELF64SYM);
    symbols = new ELF64SYM[symcnt]();
    stream->seek(symsect->offset, -1);
    if (stream->read(symbols, symsect->size) != symsect->size)
      goto err;
    // Read symbol name table
    ELF64SECT *namesect = sections + symsect->link;
    // Process relocations
    for (size_t r = 0; r < relcnt; r++) {
      ELF64RELA *rel = relocs + r;
      ELF64SYM *relsym = symbols + rel->info.sym;
      ELF64SECT *relsymsect = sections + relsym->shndx;

      char *symname;
      uintptr_t addr, offset;

      offset = sectmap[relsym->shndx] + relsym->value;

      if (relsym->info == 3) {  // Point to section
        stream->seek(sectnamesect->offset + relsymsect->name, -1);
        symname = sectnamesect != 0 ? stream->readstr() : 0;
      } else {
        stream->seek(namesect->offset + relsym->name, -1);
        symname = namesect ? stream->readstr() : 0;
      }

      if (relsymsect->type == SHT_NULL) {
        // Link library
        offset = process->linkLibrary(symname);
      }

      addr = sectbase + rel->addr;
      offset += rel->add;

      uint64_t diff64 = offset - addr;
      uint32_t diff32 = diff64;

      switch (rel->info.type) {
        case 0:   // R_X86_64_NONE
          break;
        case 1:   // R_X86_64_64
          process->writeData(addr, &offset, sizeof(uintptr_t));
          break;
        case 2:   // R_X86_64_PC32
        case 3:   // R_X86_64_GOT32
        case 4:   // R_X86_64_PLT32
          process->writeData(addr, &diff32, sizeof(uint32_t));
          break;
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
          printf("Unhandled reloc type=%x addr=%#zx name=%s%+ld = %#zx\n",
                 rel->info.type, addr, symname, rel->add, offset);
          break;
      }
    }
    delete relocs; relocs = 0;
    delete symbols; symbols = 0;
  }
  goto done;
err:
  size = 0;
done:
  delete elf;
  delete sections;
  delete sectmap;
  delete buf;
  delete symbols;
  delete relocs;
  delete relocs_buf;
  return size;
}
