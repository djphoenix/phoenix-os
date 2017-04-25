static const uint32_t MSR_EFER = 0xC0000080;
static const uint32_t MSR_STAR = 0xC0000081;
static const uint32_t MSR_LSTAR = 0xC0000082;
static const uint64_t MSR_EFER_SCE = 1 << 0;
static const uint16_t KERNEL_CS = 8;
static const uint16_t USER_CS = 24;

static inline void wrmsr(uint32_t msr_id, uint64_t msr_value) {
  asm volatile("wrmsr"::"c"(msr_id), "A"(msr_value), "d"(msr_value >> 32));
}

static inline uint64_t rdmsr(uint32_t msr_id) {
  uint32_t msr_hi, msr_lo;
  asm volatile("rdmsr":"=A"(msr_lo), "=d"(msr_hi):"c"(msr_id));
  return ((uint64_t)msr_hi << 32) | (uint64_t)msr_lo;
}
