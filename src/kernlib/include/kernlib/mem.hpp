//    PhoeniX OS Kernel library memory functions
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once

#include <stdint.h>
#include <stddef.h>

class Memory {
 public:
  static void copy(void* dest, const void* src, size_t count);
  static void fill(void *addr, uint8_t value, size_t size);
  static void zero(void *addr, size_t size);

  static void copy_aligned(void* dest, const void* src, size_t count);
  static void fill_aligned(void *addr, uint8_t value, size_t size);
  static void zero_aligned(void *addr, size_t size);
};
