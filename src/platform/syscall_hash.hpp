#pragma once

static const uint64_t hash_base = 0x467521903ff4ea37;
static const uint64_t hash_mul = 0x0064006400640064;

template<size_t L>
constexpr uint64_t syscall_hash(const char (&str)[L], size_t n = L - 1) {
  uint64_t ret = hash_base;
  for (size_t i = 0; i < n; i++) {
    ret = ret * 101 + uint64_t(str[n]) * hash_mul;
  }
  return ret;
}

static uint64_t syscall_hash(const char *str) {
  uint64_t hash = hash_base;
  while (*str) hash += hash * 101 + uint8_t(*str++) * hash_mul;
  return hash;
}
