//    PhoeniX OS Multiboot subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"

class Multiboot {
 public:
  enum Flags: uint32_t {
    MB_FLAG_MEM = (1 << 0),
    MB_FLAG_BOOTDEV = (1 << 1),
    MB_FLAG_CMDLINE = (1 << 2),
    MB_FLAG_MODS = (1 << 3),
    MB_FLAG_SYMTAB = (1 << 4),
    MB_FLAG_ELFSYMTAB = (1 << 5),
    MB_FLAG_MEMMAP = (1 << 6),
    MB_FLAG_DRIVEMAP = (1 << 7),
    MB_FLAG_CONFTAB = (1 << 8),
    MB_FLAG_BLNAME = (1 << 9),
    MB_FLAG_APMTAB = (1 << 10),
    MB_FLAG_VBETAB = (1 << 11)
  };
  struct Payload {
    Flags flags;
    size_t mem_lower:32, mem_upper:32;
    uint32_t boot_device;
    uint32_t pcmdline;
    uint32_t mods_count;
    uint32_t pmods_addr;
    uint32_t syms[3];
    uint32_t mmap_length;
    uint32_t pmmap_addr;
    uint32_t drives_length;
    uint32_t pdrives_addr;
    uint32_t pconfig_table;
    uint32_t pboot_loader_name;
    uint32_t papm_table;
    uint64_t pvbe_control_info, pvbe_mode_info, pvbe_mode, pvbe_interface_seg,
        pvbe_interface_off, pvbe_interface_len;
  } PACKED;
  struct MmapEnt {
    uint32_t size;
    void *base;
    size_t length;
    uint32_t type;
  } PACKED;
  struct Module {
    uint32_t start;
    uint32_t end;
  };

 private:
  static Payload *payload;

 public:
  static Payload *getPayload() PURE;
};
