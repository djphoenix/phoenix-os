static const uint64_t hash_base = 0x467521903ff4ea37;
static const uint64_t hash_mul = 0x0064006400640064;

template<size_t L>
constexpr uint64_t syscall_hash(const char (&str)[L], size_t n = L - 1) {
  return ((L == 0) ?
      hash_base :
      syscall_hash(str, n - 1)) * 101 +
      (str[n] * hash_mul);
}

static uint64_t syscall_hash(const char *str) {
  uint64_t hash = hash_base;
  while (*str) hash += hash * 101 + *str++ * hash_mul;
  return hash;
}
