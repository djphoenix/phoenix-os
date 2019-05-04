//    PhoeniX OS ELF binary reader
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "readelf.hpp"
#include "./readelf_internal.h"

static uintptr_t readelf_find_load_addr(Process *process,
                                   uintptr_t start,
                                   uintptr_t faddr) {
  ELF_HDR elf;
  process->readData(&elf, start, sizeof(elf));
  uintptr_t phdr_base = start + elf.phoff, phdr;
  uintptr_t phdr_top = phdr_base + elf.phnum * sizeof(ELF64_PROG);
  ELF64_PROG prog;
  for (phdr = phdr_base; phdr < phdr_top; phdr += sizeof(ELF64_PROG)) {
    process->readData(&prog, phdr, sizeof(ELF64_PROG));
    if ((prog.type == PT_LOAD) &&
        (prog.offset <= faddr) &&
        (prog.offset + prog.filesz > faddr)) break;
  }
  if (phdr == phdr_top) return 0;
  return faddr + prog.vaddr - prog.offset;
}

static bool readelf_dylink_fix_phdr(Process *process, uintptr_t start) {
  ELF_HDR elf;
  process->readData(&elf, start, sizeof(elf));

  uintptr_t phdr_base = start + elf.phoff, phdr;
  uintptr_t phdr_top = phdr_base + elf.phnum * sizeof(ELF64_PROG);
  ELF64_PROG prog;

  for (phdr = phdr_base; phdr < phdr_top; phdr += sizeof(ELF64_PROG)) {
    process->readData(&prog, phdr, sizeof(ELF64_PROG));
    if (prog.type == PT_NULL) continue;
    switch (prog.type) {
      case PT_NULL:
      case PT_LOAD:
        break;
      default:
        prog.vaddr = readelf_find_load_addr(process, start, prog.offset);
        if (prog.vaddr == 0) return 0;
        process->writeData(phdr, &prog, sizeof(ELF64_PROG));
        break;
    }
  }

  return 1;
}

static bool readelf_dylink_fix_entry(Process *process, uintptr_t start) {
  ELF_HDR elf;
  process->readData(&elf, start, sizeof(elf));
  elf.entry = readelf_find_load_addr(process, start, elf.entry);
  if (elf.entry == 0) return 0;
  process->writeData(start, &elf, sizeof(elf));
  process->setEntryAddress(elf.entry);
  return 1;
}

static bool readelf_dylink_fix_dynamic_table(Process *process,
                                                uintptr_t start,
                                                uintptr_t dyntbl,
                                                size_t dynsz) {
  uintptr_t dyntop = dyntbl + dynsz, dynptr;
  ELF64_DYN dyn;
  for (dynptr = dyntbl; dynptr < dyntop; dynptr += sizeof(ELF64_DYN)) {
    process->readData(&dyn, dynptr, sizeof(ELF64_DYN));
    switch (dyn.tag) {
      case DT_PLTGOT:
      case DT_HASH:
      case DT_STRTAB:
      case DT_SYMTAB:
      case DT_RELA:
      case DT_INIT:
      case DT_FINI:
      case DT_REL:
      case DT_DEBUG:
      case DT_JMPREL:
      case DT_INIT_ARRAY:
      case DT_FINI_ARRAY:
        dyn.val = readelf_find_load_addr(process, start, dyn.val);
        if (dyn.val == 0) return 0;
        process->writeData(dynptr, &dyn, sizeof(dyn));
        break;
      default:
        break;
    }
  }
  return 1;
}

static uint64_t readelf_dylink_dynamic_find(Process *process,
                                            uintptr_t dyntbl,
                                            size_t dynsz,
                                            ELF64_DYN_TAG tag) {
  uintptr_t dyntop = dyntbl + dynsz, dynptr;
  ELF64_DYN dyn;
  for (dynptr = dyntbl; dynptr < dyntop; dynptr += sizeof(ELF64_DYN)) {
    process->readData(&dyn, dynptr, sizeof(ELF64_DYN));
    if (dyn.tag != tag) continue;
    return dyn.val;
  }
  return 0;
}

static bool readelf_dylink_handle_dynamic_jmprel(Process *process,
                                                 uintptr_t start,
                                                 uintptr_t dyntbl,
                                                 size_t dynsz,
                                                 uintptr_t jmprel) {
  uintptr_t symtab =
      readelf_dylink_dynamic_find(process, dyntbl, dynsz, DT_SYMTAB);
  uintptr_t pltrel =
      readelf_dylink_dynamic_find(process, dyntbl, dynsz, DT_PLTREL);
  uintptr_t pltrelsz =
      readelf_dylink_dynamic_find(process, dyntbl, dynsz, DT_PLTRELSZ);
  uintptr_t syment =
      readelf_dylink_dynamic_find(process, dyntbl, dynsz, DT_SYMENT);
  uintptr_t strtab =
      readelf_dylink_dynamic_find(process, dyntbl, dynsz, DT_STRTAB);
  if (syment == 0) syment = sizeof(ELF64SYM);
  if (jmprel == 0 || symtab == 0) return 0;
  switch (pltrel) {
    case DT_REL:
      if ((pltrelsz % sizeof(ELF64REL)) != 0) return 0;
      break;
    case DT_RELA:
      if ((pltrelsz % sizeof(ELF64RELA)) != 0) return 0;
      break;
    default: return 0;
  }
  ELF64RELA rel;
  ELF64SYM sym;
  Memory::zero(&rel, sizeof(rel));
  while (pltrelsz) {
    switch (pltrel) {
      case DT_REL:
        process->readData(&rel, jmprel, sizeof(ELF64REL));
        jmprel += sizeof(ELF64REL);
        pltrelsz -= sizeof(ELF64REL);
        break;
      case DT_RELA:
        process->readData(&rel, jmprel, sizeof(ELF64RELA));
        jmprel += sizeof(ELF64RELA);
        pltrelsz -= sizeof(ELF64RELA);
        break;
      default: return 0;
    }
    uintptr_t addr = 0;
    if (rel.sym != 0) {
      process->readData(&sym, symtab + syment * rel.sym, sizeof(sym));
      if (sym.name) {
        char *name = process->readString(strtab + sym.name);
        addr = process->linkLibrary(name);
        delete name;
        if (addr == 0) {
          printf("Cannot link symbol: %s\n", name);
          return 0;
        }
      } else {
        return 0;
      }
    } else {
      printf("SYM=%016lx n=%x i=%x s=%x sz=%lx\n",
             sym.value, sym.name, sym.info, sym.shndx, sym.size);
      return 0;
    }
    addr += rel.add;
    uintptr_t ptr = readelf_find_load_addr(process, start, rel.addr);
    switch (rel.type) {
      case R_X86_64_JUMP_SLOT:
        process->writeData(ptr, &addr, sizeof(addr));
        break;
      default:
        printf("UNHANDLED REL@%#lx: %x/%x+%#lx\n",
               rel.addr, rel.type, rel.sym, rel.add);
        return 0;
    }
  }
  return 1;
}

static bool readelf_dylink_handle_dynamic_symtab(Process *process,
                                                 uintptr_t start,
                                                 uintptr_t dyntbl,
                                                 size_t dynsz,
                                                 uintptr_t symtab) {
  uintptr_t syment =
      readelf_dylink_dynamic_find(process, dyntbl, dynsz, DT_SYMENT);
  uintptr_t strtab =
      readelf_dylink_dynamic_find(process, dyntbl, dynsz, DT_STRTAB);
  uintptr_t hashtab =
      readelf_dylink_dynamic_find(process, dyntbl, dynsz, DT_HASH);
  if (syment == 0 || strtab == 0 || hashtab == 0) return 0;
  ELF64SYM sym;
  struct {
    uint32_t nbucket, nchain;
  } PACKED hashhdr;
  process->readData(&hashhdr, hashtab, sizeof(hashhdr));
  hashtab += sizeof(hashhdr);
  for (uint32_t si = 0; si < hashhdr.nbucket + hashhdr.nchain; si++) {
    uint32_t idx;
    process->readData(&idx, hashtab, sizeof(idx));
    hashtab += sizeof(idx);
    if (idx == 0) continue;
    process->readData(&sym, symtab + syment * idx, sizeof(ELF64SYM));
    if (sym.value == 0) continue;
    if (sym.name == 0) continue;
    uintptr_t ptr = readelf_find_load_addr(process, start, sym.value);
    if (ptr == 0) continue;
    char *name = process->readString(strtab + sym.name);
    if (name && strlen(name) > 0) process->addSymbol(name, ptr);
    delete name;
  }
  return 1;
}

static bool readelf_dylink_handle_dynamic_table(Process *process,
                                                uintptr_t start,
                                                uintptr_t dyntbl,
                                                size_t dynsz) {
  uintptr_t dyntop = dyntbl + dynsz, dynptr;
  ELF64_DYN dyn;
  for (dynptr = dyntbl; dynptr < dyntop; dynptr += sizeof(ELF64_DYN)) {
    process->readData(&dyn, dynptr, sizeof(ELF64_DYN));
    switch (dyn.tag) {
      case DT_JMPREL:
        if (!readelf_dylink_handle_dynamic_jmprel(
            process, start, dyntbl, dynsz, dyn.val)) return 0;
        break;
      case DT_PLTGOT:
      case DT_PLTREL:
      case DT_PLTRELSZ:
      case DT_SYMENT:
      case DT_HASH:
      case DT_GNU_HASH:
      case DT_STRTAB:
      case DT_STRSZ:
        break;
      case DT_SYMTAB:
        if (!readelf_dylink_handle_dynamic_symtab(
            process, start, dyntbl, dynsz, dyn.val)) return 0;
        break;
      case DT_NULL: break;
      default:
        printf("DYN%03lu: %016lx\n", dyn.tag, dyn.val);
        break;
    }
  }
  return 1;
}

static bool readelf_dylink_handle_dynamic(Process *process, uintptr_t start) {
  ELF_HDR elf;
  process->readData(&elf, start, sizeof(elf));

  uintptr_t phdr_base = start + elf.phoff;
  uintptr_t phdr_top = phdr_base + elf.phnum * sizeof(ELF64_PROG);
  uintptr_t phdr;
  ELF64_PROG prog;

  for (phdr = phdr_base; phdr < phdr_top; phdr += sizeof(ELF64_PROG)) {
    process->readData(&prog, phdr, sizeof(ELF64_PROG));
    if (prog.type != PT_DYNAMIC) continue;
    if (!readelf_dylink_fix_dynamic_table(
        process, start, prog.vaddr, prog.filesz) ||
        !readelf_dylink_handle_dynamic_table(
            process, start, prog.vaddr, prog.filesz))
      return 0;
  }

  return 1;
}

static bool readelf_dylink(Process *process, uintptr_t start) {
  return
      readelf_dylink_fix_phdr(process, start) &&
      readelf_dylink_fix_entry(process, start) &&
      readelf_dylink_handle_dynamic(process, start);
}

size_t readelf(Process *process, Stream *stream) {
  ELF_HDR elf;
  size_t size = 0;
  ELF64_PROG *progs = 0, *progs_top, *prog;
  char *buf = 0;
  SectionType type;
  uintptr_t vaddr, offset_load, vaddr_start = 0;

  // Read ELF header
  size_t off = 0;
  if (stream->read(&elf, sizeof(ELF_HDR)) != sizeof(ELF_HDR)) goto err;
  off += sizeof(ELF_HDR);

  // Identify ELF
  if ((elf.ident.magic != ELF_MAGIC)  // '\x7FELF'
      || (elf.ident.eclass != EC_64)
      || (elf.ident.data != ED_2LSB)
      || (elf.ident.version != EVF_CURRENT)
      || (elf.ident.osabi != 0)
      || (elf.ident.abiversion != 0)) goto err;
  // Check ELF type
  if ((elf.machine != EM_AMD64)
      || (elf.type != ET_DYN)
      || (elf.version != EV_CURRENT)
      || (elf.flags != 0)
      || (elf.ehsize != sizeof(ELF_HDR))
      || (elf.phoff != sizeof(ELF_HDR))) goto err;

  // Init variables
  size = elf.shoff + elf.shentsize * elf.shnum;
  progs = new ELF64_PROG[elf.phnum]();
  progs_top = progs + elf.phnum;

  // Read linker program
  if (stream->read(progs, sizeof(ELF64_PROG) * elf.phnum)
      != sizeof(ELF64_PROG) * elf.phnum)
    goto err;
  off += sizeof(ELF64_PROG) * elf.phnum;

  for (prog = progs; prog < progs_top; prog++) {
    switch (prog->type) {
      case PT_NULL: break;
      case PT_PHDR:
        if ((prog->offset != elf.phoff) ||
            (prog->filesz != sizeof(ELF64_PROG) * elf.phnum))
          goto err;
        break;
      case PT_LOAD:
        if (prog->flags & PF_X) {
          if (prog->flags & PF_W) goto err;
          type = SectionTypeCode;
        } else {
          if (prog->flags & PF_W)
            type = SectionTypeData;
          else
            type = SectionTypeROData;
        }
        vaddr = process->addSection(type, prog->memsz);
        if (prog->memsz > 0) {
          offset_load = 0;
          if (prog->offset == 0) {
            vaddr_start = vaddr;
            process->writeData(vaddr, &elf, sizeof(ELF_HDR));
            offset_load = sizeof(ELF_HDR) + sizeof(ELF64_PROG) * elf.phnum;
          }
          if (prog->offset + offset_load < off) goto err;
          if (prog->offset + offset_load > off) {
            stream->seek(prog->offset + offset_load - off, 0);
            off = prog->offset + offset_load;
          }
          buf = new char[prog->filesz - offset_load]();
          if (stream->read(buf, prog->filesz - offset_load) !=
              prog->filesz - offset_load) goto err;
          off += prog->filesz - offset_load;
          process->writeData(vaddr + offset_load, buf,
                             prog->filesz - offset_load);
          delete buf; buf = 0;
          prog->vaddr = vaddr;
        }
        break;
      case PT_DYNAMIC: break;
      default:
        if (prog->type >= PT_LOOS && prog->type <= PT_HIOS) break;
        if (prog->type >= PT_LOPROC && prog->type <= PT_HIPROC) break;
        goto err;
    }
  }

  process->writeData(vaddr_start + sizeof(ELF_HDR),
                     progs, sizeof(ELF64_PROG) * elf.phnum);

  goto done;
err:
  size = 0;
done:
  delete progs;
  delete buf;
  if (size != 0) {
    if (!readelf_dylink(process, vaddr_start)) size = 0;
  }
  return size;
}
