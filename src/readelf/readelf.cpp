//    PhoeniX OS ELF binary reader
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "readelf.hpp"
#include "readelf_internal.hpp"

#include "memop.hpp"
#include "sprintf.hpp"
#include "kprint.hpp"

static uintptr_t readelf_find_load_addr(Process *process, uintptr_t start, uintptr_t faddr) {
  ELF::HDR elf;
  if (!process->readData(&elf, start, sizeof(elf))) return 0;
  uintptr_t phdr_base = start + elf.phoff, phdr;
  uintptr_t phdr_top = phdr_base + elf.phnum * sizeof(ELF64::PROG);
  ELF64::PROG prog;
  for (phdr = phdr_base; phdr < phdr_top; phdr += sizeof(ELF64::PROG)) {
    if (!process->readData(&prog, phdr, sizeof(ELF64::PROG))) return 0;
    if ((prog.type == ELF64::PROG::PT_LOAD) &&
        (prog.offset <= faddr) &&
        (prog.offset + prog.filesz > faddr)) break;
  }
  if (phdr == phdr_top) return 0;
  return faddr + prog.paddr - prog.offset;
}

static uintptr_t readelf_find_load_offset(Process *process, uintptr_t start, uintptr_t faddr) {
  ELF::HDR elf;
  if (!process->readData(&elf, start, sizeof(elf))) return 0;
  uintptr_t phdr_base = start + elf.phoff, phdr;
  uintptr_t phdr_top = phdr_base + elf.phnum * sizeof(ELF64::PROG);
  ELF64::PROG prog;
  for (phdr = phdr_base; phdr < phdr_top; phdr += sizeof(ELF64::PROG)) {
    if (!process->readData(&prog, phdr, sizeof(ELF64::PROG))) return 0;
    if ((prog.type == ELF64::PROG::PT_LOAD) &&
        (prog.vaddr <= faddr) &&
        (prog.vaddr + prog.filesz > faddr)) break;
  }
  if (phdr == phdr_top) return 0;
  return faddr + prog.offset - prog.vaddr;
}

static bool readelf_dylink_fix_phdr(Process *process, uintptr_t start) {
  ELF::HDR elf;
  if (!process->readData(&elf, start, sizeof(elf))) return false;

  uintptr_t phdr_base = start + elf.phoff, phdr;
  uintptr_t phdr_top = phdr_base + elf.phnum * sizeof(ELF64::PROG);
  ELF64::PROG prog;

  for (phdr = phdr_base; phdr < phdr_top; phdr += sizeof(ELF64::PROG)) {
    if (!process->readData(&prog, phdr, sizeof(ELF64::PROG))) return false;
    switch (prog.type) {
      case ELF64::PROG::PT_NULL:
      case ELF64::PROG::PT_LOAD:
        break;
      default:
        prog.paddr = readelf_find_load_addr(process, start, prog.offset);
        if (prog.paddr == 0) return 0;
        process->writeData(phdr, &prog, sizeof(ELF64::PROG));
        break;
    }
  }

  return 1;
}

static bool readelf_dylink_fix_entry(Process *process, uintptr_t start) {
  ELF::HDR elf;
  if (!process->readData(&elf, start, sizeof(elf))) return false;
  uintptr_t offset = readelf_find_load_offset(process, start, elf.entry);
  uintptr_t entry = readelf_find_load_addr(process, start, offset);
  if (entry == 0) return 0;
  elf.entry = entry;
  process->writeData(start, &elf, sizeof(elf));
  process->setEntryAddress(elf.entry);
  return 1;
}

static bool readelf_dylink_fix_dynamic_table(Process *process, uintptr_t start,
                                             uintptr_t dyntbl, size_t dynsz) {
  uintptr_t dyntop = dyntbl + dynsz, dynptr;
  ELF64::DYN dyn;
  uintptr_t offset;
  for (dynptr = dyntbl; dynptr < dyntop; dynptr += sizeof(ELF64::DYN)) {
    if (!process->readData(&dyn, dynptr, sizeof(ELF64::DYN))) return false;
    switch (dyn.tag) {
      case ELF64::DYN::DT_PLTGOT:
      case ELF64::DYN::DT_HASH:
      case ELF64::DYN::DT_STRTAB:
      case ELF64::DYN::DT_SYMTAB:
      case ELF64::DYN::DT_RELA:
      case ELF64::DYN::DT_INIT:
      case ELF64::DYN::DT_FINI:
      case ELF64::DYN::DT_REL:
      case ELF64::DYN::DT_DEBUG:
      case ELF64::DYN::DT_JMPREL:
      case ELF64::DYN::DT_INIT_ARRAY:
      case ELF64::DYN::DT_FINI_ARRAY:
        offset = readelf_find_load_offset(process, start, dyn.val);
        dyn.val = readelf_find_load_addr(process, start, offset);
        if (dyn.val == 0) return 0;
        process->writeData(dynptr, &dyn, sizeof(dyn));
        break;
      default:
        break;
    }
  }
  return 1;
}

static uint64_t readelf_dylink_dynamic_find(Process *process, uintptr_t dyntbl,
                                            size_t dynsz, ELF64::DYN::TAG tag) {
  uintptr_t dyntop = dyntbl + dynsz, dynptr;
  ELF64::DYN dyn;
  for (dynptr = dyntbl; dynptr < dyntop; dynptr += sizeof(ELF64::DYN)) {
    if (!process->readData(&dyn, dynptr, sizeof(ELF64::DYN))) return 0;
    if (dyn.tag != tag) continue;
    return dyn.val;
  }
  return 0;
}

static bool readelf_dylink_process_reloc(Process *process, KernelLinker *linker, uintptr_t start, const ELF64::RELA &ent,
                                         uintptr_t symtab, uintptr_t syment, uintptr_t strtab) {
  if (ent.sym == 0) return 0;
  ELF64::SYM sym;
  if (!process->readData(&sym, symtab + syment * ent.sym, sizeof(sym))) return false;
  if (!sym.name) return 0;

  uintptr_t addr = 0;
  ptr<char> name(process->readString(strtab + sym.name));
  uintptr_t offset = readelf_find_load_offset(process, start, ent.addr);
  uintptr_t ptr = readelf_find_load_addr(process, start, offset);
  char printbuf[80];

  bool allownull = sym.bind == ELF64::SYM::STB_WEAK;
  addr = linker->getSymbolByName(name.get());
  if (!addr) addr = linker->linkLibrary(name.get());
  if ((!addr) && (offset = readelf_find_load_offset(process, start, sym.value)) != 0) {
    addr = readelf_find_load_addr(process, start, offset);
  }

  if (!allownull && addr == 0) {
    snprintf(printbuf, sizeof(printbuf), "Cannot link symbol: %s\n", name.get());
    kprint(printbuf);
    return 0;
  }

  addr += ent.add;
  switch (ent.type) {
    case ELF64::R_X86_64_JUMP_SLOT:
    case ELF64::R_X86_64_GLOB_DAT:
      process->writeData(ptr, &addr, sizeof(addr));
      break;
    default:
      snprintf(printbuf, sizeof(printbuf), "UNHANDLED REL@%#lx: %x => %x+%#lx\n", ent.addr, ent.type, ent.sym, ent.add);
      kprint(printbuf);
      return 0;
  }
  return 1;
}

static bool readelf_dylink_handle_dynamic_jmprel(Process *process, KernelLinker *linker, uintptr_t start,
                                                 uintptr_t dyntbl, size_t dynsz, uintptr_t jmprel) {
  uintptr_t symtab = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_SYMTAB);
  uintptr_t pltrel = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_PLTREL);
  uintptr_t pltrelsz = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_PLTRELSZ);
  uintptr_t syment = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_SYMENT);
  uintptr_t strtab = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_STRTAB);
  if (syment == 0) syment = sizeof(ELF64::SYM);
  if (jmprel == 0 || symtab == 0) return 0;
  size_t entsz;
  switch (pltrel) {
    case ELF64::DYN::DT_REL: entsz = sizeof(ELF64::REL); break;
    case ELF64::DYN::DT_RELA: entsz = sizeof(ELF64::RELA); break;
    default: return 0;
  }
  if ((pltrelsz % entsz) != 0) return 0;
  ELF64::RELA rel;
  Memory::zero(&rel, sizeof(rel));
  while (pltrelsz) {
    if (!process->readData(&rel, jmprel, entsz)) return false;
    jmprel += entsz;
    pltrelsz -= entsz;
    if (!readelf_dylink_process_reloc(process, linker, start, rel, symtab, syment, strtab)) return 0;
  }
  return true;
}

static bool readelf_dylink_handle_dynamic_rel(Process *process, KernelLinker *linker, uintptr_t start,
                                              uintptr_t dyntbl, size_t dynsz, uintptr_t rel) {
  uintptr_t sz = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_RELSZ);
  uintptr_t entsz = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_RELENT);
  uintptr_t symtab = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_SYMTAB);
  uintptr_t syment = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_SYMENT);
  uintptr_t strtab = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_STRTAB);
  if (sz == 0 || entsz != sizeof(ELF64::REL)) return 0;
  ELF64::RELA ent;
  ent.add = 0;
  while (sz) {
    if (!process->readData(&ent, rel, sizeof(ELF64::REL))) return false;
    rel += entsz;
    sz -= entsz;
    if (!readelf_dylink_process_reloc(process, linker, start, ent, symtab, syment, strtab)) return 0;
  }
  return true;
}

static bool readelf_dylink_handle_dynamic_rela(Process *process, KernelLinker *linker, uintptr_t start,
                                               uintptr_t dyntbl, size_t dynsz, uintptr_t rela) {
  uintptr_t sz = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_RELASZ);
  uintptr_t entsz = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_RELAENT);
  uintptr_t symtab = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_SYMTAB);
  uintptr_t syment = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_SYMENT);
  uintptr_t strtab = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_STRTAB);
  if (sz == 0 || entsz != sizeof(ELF64::RELA)) return 0;
  ELF64::RELA ent;
  while (sz) {
    if (!process->readData(&ent, rela, sizeof(ent))) return false;
    rela += entsz;
    sz -= entsz;
    if (!readelf_dylink_process_reloc(process, linker, start, ent, symtab, syment, strtab)) return 0;
  }
  return 1;
}

static bool readelf_dylink_handle_dynamic_symtab(Process *process, KernelLinker *linker, uintptr_t start,
                                                 uintptr_t dyntbl, size_t dynsz, uintptr_t symtab) {
  uintptr_t syment = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_SYMENT);
  uintptr_t strtab = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_STRTAB);
  uintptr_t hashtab = readelf_dylink_dynamic_find(process, dyntbl, dynsz, ELF64::DYN::DT_HASH);
  if (syment == 0 || strtab == 0 || hashtab == 0) return 0;
  ELF64::SYM sym;
  struct {
    uint32_t nbucket, nchain;
  } __attribute__((packed)) hashhdr;
  if (!process->readData(&hashhdr, hashtab, sizeof(hashhdr))) return false;
  hashtab += sizeof(hashhdr);
  ptr<char> name;
  for (uint32_t si = 0; si < hashhdr.nbucket + hashhdr.nchain; si++) {
    uint32_t idx;
    if (!process->readData(&idx, hashtab, sizeof(idx))) return false;
    hashtab += sizeof(idx);
    if (idx == 0) continue;
    if (!process->readData(&sym, symtab + syment * idx, sizeof(ELF64::SYM))) return false;
    if (sym.name == 0 || sym.value == 0) continue;
    uintptr_t ptr = readelf_find_load_addr(process, start, sym.value);
    if (ptr == 0) continue;
    name = process->readString(strtab + sym.name);
    if (name && name[0] != 0) linker->addSymbol(name.get(), ptr);
  }
  return 1;
}

static bool readelf_dylink_handle_dynamic_table(Process *process, KernelLinker *linker, uintptr_t start,
                                                uintptr_t dyntbl, size_t dynsz) {
  uintptr_t dyntop = dyntbl + dynsz, dynptr;
  ELF64::DYN dyn;
  for (dynptr = dyntbl; dynptr < dyntop; dynptr += sizeof(ELF64::DYN)) {
    if (!process->readData(&dyn, dynptr, sizeof(ELF64::DYN))) return false;
    switch (dyn.tag) {
      case ELF64::DYN::DT_JMPREL:
        if (!readelf_dylink_handle_dynamic_jmprel(process, linker, start, dyntbl, dynsz, dyn.val)) return 0;
        break;
      case ELF64::DYN::DT_REL:
        if (!readelf_dylink_handle_dynamic_rel(process, linker, start, dyntbl, dynsz, dyn.val)) return 0;
        break;
      case ELF64::DYN::DT_RELA:
        if (!readelf_dylink_handle_dynamic_rela(process, linker, start, dyntbl, dynsz, dyn.val)) return 0;
        break;
      case ELF64::DYN::DT_SYMTAB:
        if (!readelf_dylink_handle_dynamic_symtab(process, linker, start, dyntbl, dynsz, dyn.val)) return 0;
        break;
      case ELF64::DYN::DT_PLTGOT:
      case ELF64::DYN::DT_PLTREL:
      case ELF64::DYN::DT_PLTRELSZ:
      case ELF64::DYN::DT_RELASZ:
      case ELF64::DYN::DT_RELAENT:
      case ELF64::DYN::DT_RELSZ:
      case ELF64::DYN::DT_RELENT:
      case ELF64::DYN::DT_SYMENT:
      case ELF64::DYN::DT_HASH:
      case ELF64::DYN::DT_GNU_HASH:
      case ELF64::DYN::DT_STRTAB:
      case ELF64::DYN::DT_STRSZ:
      case ELF64::DYN::DT_NULL:
        break;
      default:
        char printbuf[80];
        snprintf(printbuf, sizeof(printbuf), "Unhandled DYN%03lu: %016lx\n", dyn.tag, dyn.val);
        kprint(printbuf);
        break;
    }
  }
  return 1;
}

static bool readelf_dylink_handle_dynamic(Process *process, KernelLinker *linker, uintptr_t start) {
  ELF::HDR elf;
  if (!process->readData(&elf, start, sizeof(elf))) return false;

  uintptr_t phdr_base = start + elf.phoff;
  uintptr_t phdr_top = phdr_base + elf.phnum * sizeof(ELF64::PROG);
  uintptr_t phdr;
  ELF64::PROG prog;

  for (phdr = phdr_base; phdr < phdr_top; phdr += sizeof(ELF64::PROG)) {
    if (!process->readData(&prog, phdr, sizeof(ELF64::PROG))) return false;
    if (prog.type != ELF64::PROG::PT_DYNAMIC) continue;
    if (!readelf_dylink_fix_dynamic_table(process, start, prog.paddr, prog.filesz) ||
        !readelf_dylink_handle_dynamic_table(process, linker, start, prog.paddr, prog.filesz))
      return 0;
  }

  return 1;
}

static bool readelf_dylink(Process *process, KernelLinker *linker, uintptr_t start) {
  return
      readelf_dylink_fix_phdr(process, start) &&
      readelf_dylink_fix_entry(process, start) &&
      readelf_dylink_handle_dynamic(process, linker, start);
}

size_t readelf(Process *process, KernelLinker *linker, const void *mem, size_t memsize) {
  const uint8_t *elfbase = reinterpret_cast<const uint8_t*>(mem);
  ELF::HDR elf;
  size_t size = 0;
  ELF64::PROG *progs_top, *prog;
  ptr<ELF64::PROG> progs;
  SectionType type;
  uintptr_t paddr, palign, paddr_start = 0;

  // Read ELF header
  Memory::copy(&elf, elfbase, sizeof(ELF::HDR)); // TODO: validate

  // Identify ELF
  if ((elf.ident.magic != ELF::IDENT::MAGIC)  // '\x7FELF'
      || (elf.ident.eclass != ELF::EC_64)
      || (elf.ident.data != ELF::ED_2LSB)
      || (elf.ident.version != ELF::EVF_CURRENT)
      || (elf.ident.osabi != 0)
      || (elf.ident.abiversion != 0)) return 0;
  // Check ELF type
  if ((elf.machine != ELF::EM_AMD64)
      || (elf.type != ELF::ET_DYN)
      || (elf.version != ELF::EV_CURRENT)
      || (elf.flags != 0)
      || (elf.ehsize != sizeof(ELF::HDR))
      || (elf.phoff != sizeof(ELF::HDR))) return 0;

  // Init variables
  size = elf.shoff + elf.shentsize * elf.shnum;
  progs = new ELF64::PROG[elf.phnum];
  progs_top = progs.get() + elf.phnum;

  // Read linker program
  Memory::copy(progs.get(), elfbase + sizeof(ELF::HDR), sizeof(ELF64::PROG) * elf.phnum); // TODO: validate

  for (prog = progs.get(); prog < progs_top; prog++) {
    switch (prog->type) {
      case ELF64::PROG::PT_NULL: break;
      case ELF64::PROG::PT_PHDR:
        if ((prog->offset != elf.phoff) || (prog->filesz != sizeof(ELF64::PROG) * elf.phnum)) return 0;
        break;
      case ELF64::PROG::PT_LOAD:
        if (prog->flags & ELF64::PROG::PF_X) {
          if (prog->flags & ELF64::PROG::PF_W) return 0;
          type = SectionTypeCode;
        } else {
          if (prog->flags & ELF64::PROG::PF_W)
            type = SectionTypeData;
          else
            type = SectionTypeROData;
        }
        palign = prog->offset & 0xFFF;
        paddr = process->addSection(prog->vaddr & (~0xFFFllu), type, prog->memsz + palign) + palign;
        if (prog->memsz > 0) {
          if (prog->offset == 0) paddr_start = paddr;
          process->writeData(paddr, elfbase + prog->offset, prog->filesz); // TODO: validate
          prog->paddr = paddr;
        }
        break;
      case ELF64::PROG::PT_DYNAMIC: break;
      default:
        if (prog->type >= ELF64::PROG::PT_LOOS && prog->type <= ELF64::PROG::PT_HIOS) break;
        if (prog->type >= ELF64::PROG::PT_LOPROC && prog->type <= ELF64::PROG::PT_HIPROC) break;
        return 0;
    }
  }

  process->writeData(paddr_start, &elf, sizeof(ELF::HDR));
  process->writeData(paddr_start + sizeof(ELF::HDR), progs.get(), sizeof(ELF64::PROG) * elf.phnum);

  if (!readelf_dylink(process, linker, paddr_start)) return 0;
  return size;
}
