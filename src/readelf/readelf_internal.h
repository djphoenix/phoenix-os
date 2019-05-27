namespace ELF {
  enum CLASS: uint8_t {
    EC_NONE = 0,
    EC_32 = 1,
    EC_64 = 2
  };
  enum DATA: uint8_t {
    ED_NONE = 0,
    ED_2LSB = 1,
    ED_2MSB = 2
  };
  enum FILE_VERSION: uint8_t {
    EVF_NONE = 0,
    EVF_CURRENT = 1
  };
  enum OSABI: uint8_t { };
  enum ABIVERSION: uint8_t { };
  enum TYPE: uint16_t {
    ET_NONE = 0,
    ET_REL = 1,
    ET_EXEC = 2,
    ET_DYN = 3,
    ET_CORE = 4
  };
  enum MACHINE: uint16_t {
    EM_NONE = 0,
    EM_386 = 3,
    EM_AMD64 = 62
  };
  enum VERSION: uint32_t {
    EV_NONE = 0,
    EV_CURRENT = 1
  };

  struct IDENT {
    static const uint32_t MAGIC = 0x464C457F;  // '\x7fELF'
    uint32_t magic;
    CLASS eclass;
    DATA data;
    FILE_VERSION version;
    OSABI osabi;
    ABIVERSION abiversion;
    uint8_t pad[7];
  } PACKED;
  struct HDR {
    IDENT ident;
    TYPE type;
    MACHINE machine;
    VERSION version;
    uint64_t entry, phoff, shoff;
    uint32_t flags;
    uint16_t ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
  } PACKED;

  namespace _64 {
    struct PROG {
      enum TYPE: uint32_t {
        PT_NULL,
        PT_LOAD,
        PT_DYNAMIC,
        PT_INTERP,
        PT_NOTE,
        PT_SHLIB,
        PT_PHDR,
        PT_TLS,
        PT_LOOS = 0x60000000,
        PT_HIOS = 0x6FFFFFFF,
        PT_LOPROC = 0x70000000,
        PT_HIPROC = 0x7FFFFFFF
      };
      enum FLAGS: uint32_t {
        PF_X = 1,
        PF_W = 2,
        PF_R = 4
      };
      TYPE type;
      FLAGS flags;
      uint64_t offset;
      uint64_t vaddr;
      uint64_t paddr;
      uint64_t filesz;
      uint64_t memsz;
      uint64_t align;
    } PACKED;

    struct SECT {
      enum TYPE: uint32_t {
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
      enum FLAGS: uint64_t {
        SHF_WRITE = 1,
        SHF_ALLOC = 2,
        SHF_EXECINSTR = 4
      };
      uint32_t name;
      TYPE type;
      FLAGS flags;
      uint64_t addr, offset, size;
      uint32_t link, info;
      uint64_t addralign, entsize;
    } PACKED;

    struct DYN {
      enum TAG: uint64_t {
        DT_NULL,
        DT_NEEDED,
        DT_PLTRELSZ,
        DT_PLTGOT,
        DT_HASH,
        DT_STRTAB,
        DT_SYMTAB,
        DT_RELA,
        DT_RELASZ,
        DT_RELAENT,
        DT_STRSZ,
        DT_SYMENT,
        DT_INIT,
        DT_FINI,
        DT_SONAME,
        DT_RPATH,
        DT_SYMBOLIC,
        DT_REL,
        DT_RELSZ,
        DT_RELENT,
        DT_PLTREL,
        DT_DEBUG,
        DT_TEXTREL,
        DT_JMPREL,
        DT_BIND_NOW,
        DT_INIT_ARRAY,
        DT_FINI_ARRAY,
        DT_INIT_ARRAYSZ,
        DT_FINI_ARRAYSZ,
        DT_RUNPATH,
        DT_FLAGS,
        DT_ENCODING,
        DT_PREINIT_ARRAY,
        DT_PREINIT_ARRAYSZ,
        DT_MAXPOSTAGS,
        DT_LOOS = 0x60000000,
        DT_GNU_HASH = 0x6FFFFEF5,
        DT_HIOS = 0x6FFFFFFF,
        DT_LOPROC = 0x70000000,
        DT_HIPROC = 0x7FFFFFFF
      };
      TAG tag;
      uint64_t val;
    } PACKED;

    struct SYM {
      enum BIND: uint8_t {
        STB_LOCAL = 0,
        STB_GLOBAL = 1,
        STB_WEAK = 2,
        STB_LOOS = 10,
        STB_HIOS = 12,
        STB_LOPROC = 13,
        STB_HIPROC = 15,
      };
      enum TYPE: uint8_t {
        STT_NOTYPE = 0,
        STT_OBJECT = 1,
        STT_FUNC = 2,
        STT_SECTION = 3,
        STT_FILE = 4,
        STT_COMMON = 5,
        STT_TLS = 6,
        STT_LOOS = 10,
        STT_HIOS = 12,
        STT_LOPROC = 13,
        STT_SPARC_REGISTER = 13,
        STT_HIPROC = 15,
      };
      uint32_t name;
      BIND bind:4; TYPE type:4;
      uint8_t other;
      uint16_t shndx;
      uint64_t value;
      uint64_t size;
    } PACKED;

    enum RELOC_TYPE: uint32_t {
      R_X86_64_NONE,       // No reloc
      R_X86_64_64,         // Direct 64 bit
      R_X86_64_PC32,       // PC relative 32 bit signed
      R_X86_64_GOT32,      // 32 bit GOT entry
      R_X86_64_PLT32,      // 32 bit PLT address
      R_X86_64_COPY,       // Copy symbol at runtime
      R_X86_64_GLOB_DAT,   // Create GOT entry
      R_X86_64_JUMP_SLOT,  // Create PLT entry
      R_X86_64_RELATIVE,   // Adjust by program base
      R_X86_64_GOTPCREL,   // 32 bit signed pc relative offset to GOT
      R_X86_64_32,         // Direct 32 bit zero extended
      R_X86_64_32S,        // Direct 32 bit sign extended
      R_X86_64_16,         // Direct 16 bit zero extended
      R_X86_64_PC16,       // 16 bit sign extended pc relative
      R_X86_64_8,          // Direct 8 bit sign extended
      R_X86_64_PC8         // 8 bit sign extended pc relative
    };
    struct REL {
      uint64_t addr;
      RELOC_TYPE type;
      uint32_t sym;
    } PACKED;
    struct RELA : public REL {
      uint64_t add;
    } PACKED;
  }
}
namespace ELF64 = ELF::_64;
