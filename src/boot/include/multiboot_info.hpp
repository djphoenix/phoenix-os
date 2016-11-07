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
#include "pxlib.hpp"

struct GRUBMODULE {
  uint32_t start;
  uint32_t end;
};

struct GRUB {
  uint32_t flags;
  uint32_t mem_lower, mem_upper;
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
};

struct GRUBMEMENT {
  uint32_t size;
  void *base;
  size_t length;
  uint32_t type;
} PACKED;

struct MODULE {
  void* start;
  void* end;
  MODULE* next;
};

struct GRUBDATA {
  uint32_t flags;
  uint32_t mem_lower, mem_upper;
  uint32_t boot_device;
  char* cmdline;
  MODULE *mods;
  size_t mmap_length;
  char* mmap_addr;
  char* boot_loader_name;
  uintptr_t kernel, stack, stack_top, data, data_top, bss, bss_top, modules,
      modules_top;
};

extern GRUB *grub_data;

extern GRUBDATA kernel_data;
