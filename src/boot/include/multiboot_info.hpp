//    PhoeniX OS Multiboot subsystem
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

#pragma once
#include "kernlib.hpp"

enum MUTLIBOOT_FLAGS: uint32_t {
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

struct MULTIBOOT_PAYLOAD {
  MUTLIBOOT_FLAGS flags;
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

struct MULTIBOOT_MODULE {
  uint32_t start;
  uint32_t end;
};

struct MULTIBOOT_MMAP_ENT {
  uint32_t size;
  void *base;
  size_t length;
  uint32_t type;
} PACKED;

class Multiboot {
 private:
  static MULTIBOOT_PAYLOAD *payload;
 public:
  static MULTIBOOT_PAYLOAD *getPayload();
};
