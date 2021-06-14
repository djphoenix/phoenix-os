#pragma once

static const uint64_t hash_base = 0x467521903ff4ea37;
static const uint64_t hash_mul = 0x0064006400640064;

template<size_t L>
constexpr uint64_t syscall_hash_ce(const char (&str)[L]) {
  uint64_t hash = hash_base;
  for (size_t i = 0; i < L-1; i++) {
    hash += hash * 101 + uint64_t(uint8_t(str[i]) & 0xFF) * hash_mul;
  }
  return hash;
}

static uint64_t syscall_hash(const char *str) {
  uint64_t hash = hash_base;
  while (*str) {
    hash += hash * 101 + uint64_t(uint8_t(*(str++)) & 0xFF) * hash_mul;
  }
  return hash;
}
