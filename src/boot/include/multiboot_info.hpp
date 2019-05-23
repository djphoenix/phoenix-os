//    PhoeniX OS Multiboot subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"

class Multiboot {
 public:
  enum Flags: uint32_t {
    FLAG_MEM = (1 << 0),
    FLAG_BOOTDEV = (1 << 1),
    FLAG_CMDLINE = (1 << 2),
    FLAG_MODS = (1 << 3),
    FLAG_SYMTAB = (1 << 4),
    FLAG_ELFSYMTAB = (1 << 5),
    FLAG_MEMMAP = (1 << 6),
    FLAG_DRIVEMAP = (1 << 7),
    FLAG_CONFTAB = (1 << 8),
    FLAG_BLNAME = (1 << 9),
    FLAG_APMTAB = (1 << 10),
    FLAG_VBETAB = (1 << 11),
    FLAG_FBTAB = (1 << 12),
  };
  enum MemoryType: uint32_t {
    MEMORY_AVAILABLE = 1,
    MEMORY_RESERVED = 2,
    MEMORY_ACPI_RECLAIMABLE = 3,
    MEMORY_NVS = 4,
    MEMORY_BADRAM = 5,
  };
  enum FramebufferType: uint8_t {
    FRAMEBUFFER_TYPE_INDEXED = 0,
    FRAMEBUFFER_TYPE_RGB = 1,
    FRAMEBUFFER_TYPE_EGA_TEXT = 2,
  };
  struct Payload {
    Flags flags;
    struct {
      size_t lower:32, upper:32;
    } PACKED mem;
    uint32_t boot_device;
    uintptr_t pcmdline:32;
    struct { uint32_t count; uintptr_t paddr:32; } PACKED mods;
    union {
      struct { uint32_t tabsize, strsize, addr, rsvd; } PACKED aout;
      struct { uint32_t num, size, addr, shndx; } PACKED elf;
    } PACKED syms;
    struct { size_t size:32; uintptr_t paddr:32; } PACKED mmap;
    struct { size_t size:32; uintptr_t paddr:32; } PACKED drives;
    uintptr_t pconfig_table:32;
    uintptr_t pboot_loader_name:32;
    uintptr_t papm_table:32;
    struct {
      uintptr_t pcontrol_info:32;
      uintptr_t pmode_info:32;
      uint16_t mode, interface_seg, interface_off, interface_len;
    } PACKED vbe;
    struct {
      uintptr_t paddr;
      uint32_t pitch, width, height;
      uint8_t bpp;
      FramebufferType type;
      union {
        struct {
          uint32_t paddr;
          uint16_t num_colors;
        } PACKED palette;
        struct {
          uint8_t red_field_position;
          uint8_t red_mask_size;
          uint8_t green_field_position;
          uint8_t green_mask_size;
          uint8_t blue_field_position;
          uint8_t blue_mask_size;
        } PACKED rgb;
      } PACKED;
    } PACKED fb;
  } PACKED;
  struct VBEInfo {
    uint32_t signature;
    uint16_t version;
    uintptr_t vendor_string:32;
    uint32_t capabilities;
    uintptr_t video_mode_ptr:32;
    uint16_t total_memory;

    uint16_t oem_software_rev;
    uintptr_t oem_vendor_name_ptr:32;
    uintptr_t oem_product_name_ptr:32;
    uintptr_t oem_product_rev_ptr:32;

    uint8_t reserved[222];
    uint8_t oem_data[256];
  } PACKED;
  struct VBEModeInfo {
      uint16_t mode_attr;
      uint8_t win_attr[2];
      uint16_t win_grain;
      uint16_t win_size;
      uint16_t win_seg[2];
      uintptr_t win_scheme:32;
      uint16_t logical_scan;

      uint16_t h_res;
      uint16_t v_res;
      uint8_t char_width;
      uint8_t char_height;
      uint8_t memory_planes;
      uint8_t bpp;
      uint8_t banks;
      uint8_t memory_layout;
      uint8_t bank_size;
      uint8_t image_pages;
      uint8_t page_function;

      uint8_t rmask;
      uint8_t rpos;
      uint8_t gmask;
      uint8_t gpos;
      uint8_t bmask;
      uint8_t bpos;
      uint8_t resv_mask;
      uint8_t resv_pos;
      uint8_t dcm_info;

      uintptr_t lfb_ptr:32;
      uintptr_t offscreen_ptr:32;
      uint16_t offscreen_size;

      uint8_t reserved[206];
  } PACKED;
  struct MmapEnt {
    uint32_t size;
    void *base;
    size_t length;
    MemoryType type;
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
