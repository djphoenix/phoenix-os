//    PhoeniX OS CPUID subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "cpu.hpp"
#include "kernlib/sprintf.hpp"

uint32_t CPU::vendor[4] {};
uint32_t CPU::brandString[13] {};
uint64_t CPU::features = 0;
uint64_t CPU::features_ext = 0;
uint64_t CPU::ext_features = 0;
CPU::Info CPU::info = { 0xFFFFFFFF, 0, 0, 0 };
uint32_t CPU::maxCPUID = 0;

static const char CPUID_FEAT_STR[64][16] = {
  /* 00-03 */"fpu", "vme", "de", "pse",
  /* 04-07 */"tsc", "msr", "pae", "mce",
  /* 08-11 */"cx8", "apic", "", "sep",
  /* 12-15 */"mtrr", "pge", "mca", "cmov",
  /* 16-19 */"pat", "pse36", "psn", "clfsh",
  /* 20-23 */"", "ds", "acpi", "mmx",
  /* 24-27 */"fxsr", "sse", "sse2", "ss",
  /* 28-31 */"htt", "tm", "ia64", "pbe",

  /* 32-35 */"sse3", "pclmul", "dtes64", "monitor",
  /* 36-39 */"ds-cpl", "vmx", "smx", "est",
  /* 40-43 */"tm2", "ssse3", "cnxt-id", "sdbg",
  /* 44-47 */"fma", "cx16", "xtpr", "pdcm",
  /* 48-51 */"", "pcid", "dca", "sse4.1",
  /* 52-55 */"sse4.2", "x2apic", "movbe", "popcnt",
  /* 56-59 */"tsc-dl", "aes", "xsave", "osxsave",
  /* 60-63 */"avx", "f16c", "rdrnd", "hv"
};

static const char CPUID_FEAT_EXT_STR[64][16] = {
  /* 00-03 */"fsgsbase", "ia32tscadj", "sgx", "bmi1",
  /* 04-07 */"hle", "avx2", "", "smep",
  /* 08-11 */"bmi2", "erms", "invpcid", "rtm",
  /* 12-15 */"pqm", "fpucsds", "mpx", "pqe",
  /* 16-19 */"avx512f", "avx512dq", "rdseed", "adx",
  /* 20-23 */"smap", "avx512ifma", "pcommit", "clflushopt",
  /* 24-27 */"clwb", "intel_pt", "avx512pf", "avx512er",
  /* 28-31 */"avx512cd", "sha", "avx512bw", "avx512vl",

  /* 32-35 */"prefetchwt1", "avx512vbmi", "", "pku",
  /* 36-39 */"ospke", "", "", "",
  /* 40-43 */"", "", "", "",
  /* 44-47 */"", "", "", "",
  /* 48-51 */"", "", "", "",
  /* 52-55 */"", "", "", "",
  /* 56-59 */"", "", "", "",
  /* 60-63 */"", "", "", "",
};

static const char CPUID_EXT_FEAT_STR[64][16] = {
  /* 00-03 */"fpu", "vme", "de", "pse",
  /* 04-07 */"tsc", "msr", "pae", "mce",
  /* 08-11 */"cx8", "apic", "", "syscall",
  /* 12-15 */"mtrr", "pge", "mca", "cmov",
  /* 16-19 */"pat", "pse36", "", "mp",
  /* 20-23 */"nx", "", "mmxext", "mmx",
  /* 24-27 */"fxsr", "fxsr_opt", "pdpe1gb", "rdtscp",
  /* 28-31 */"", "lm", "3dnowext", "3dnow",

  /* 32-35 */"lahf_lm", "cmp_legacy", "svm", "extapic",
  /* 36-39 */"cr8_legacy", "abm", "sse4a", "misalignsse",
  /* 40-43 */"3dnowprefetch", "osvw", "ibs", "xop",
  /* 44-47 */"skinit", "wdt", "", "lwp",
  /* 48-51 */"fma4", "tce", "", "nodeid_msr",
  /* 52-55 */"", "tbm", "topoext", "perfctr_core",
  /* 56-59 */"perfctr_nb", "", "dbx", "perftsc",
  /* 60-63 */"pcx_l2i", "", "", ""
};

const char* CPU::getVendor() {
  if (vendor[0] == 0) {
    uint32_t eax = 0;
    asm volatile("cpuid" : "+a"(eax), "=b"(vendor[0]), "=d"(vendor[1]), "=c"(vendor[2]));
    vendor[3] = 0;
  }
  return reinterpret_cast<const char*>(vendor);
}

uint32_t CPU::getMaxCPUID() {
  if (maxCPUID == 0) {
    uint32_t eax = 0x80000000;
    asm volatile("cpuid" : "+a"(eax)::"ecx","ebx","edx");
    maxCPUID = eax;
  }
  return maxCPUID;
}

uint64_t CPU::getFeatures() {
  if (features == 0) {
    uint32_t eax = 1, ecx, edx;
    asm volatile("cpuid" : "+a"(eax), "=c"(ecx), "=d"(edx) :: "ebx");
    features = (uint64_t(ecx) << 32) | uint64_t(edx);
    struct CPUID_INFO {
      uint32_t stepping:3;
      uint32_t model:4;
      uint32_t family:4;
      uint32_t type:2;
      uint32_t model_ext:4;
      uint32_t family_ext:8;
    };
    CPUID_INFO *cpuid_info = reinterpret_cast<CPUID_INFO*>(&eax);
    info.stepping = cpuid_info->stepping;
    info.type = cpuid_info->type;
    info.model = cpuid_info->model;
    info.family = cpuid_info->family;
    if (info.family == 6 || info.family == 15) {
      info.family += cpuid_info->family_ext;
      info.model += uint32_t(cpuid_info->model_ext << 4);
    }
  }
  return features;
}

uint64_t CPU::getFeaturesExt() {
  if (features_ext == 0) {
    uint32_t eax = 7, ebx, ecx = 0;
    asm volatile("cpuid" : "+a"(eax), "=b"(ebx), "+c"(ecx) :: "edx");
    features_ext = uint64_t(ebx) | (uint64_t(ecx) << 32);
  }
  return features_ext;
}

uint64_t CPU::getExtFeatures() {
  if (ext_features == 0 && (getMaxCPUID() >= 0x80000001)) {
    uint32_t eax = 0x80000001, ecx, edx;
    asm volatile("cpuid" : "+a"(eax), "=c"(ecx), "=d"(edx) :: "ebx");
    ext_features = uint64_t(edx) | (uint64_t(ecx) << 32);
  }
  return ext_features;
}

CPU::Info CPU::getInfo() {
  if (info.type == 0xFFFFFFFF)
    getFeatures();
  return info;
}

static inline uint32_t bitcnt(uint64_t val) {
  uint32_t res = 0;
  for (uint32_t b = 0; b < 64; b++)
    res += (val >> b) & 1;
  return res;
}

uint64_t CPU::getFeaturesStrSize() {
  uint64_t f = getFeatures(), ef = getExtFeatures(), fe = getFeaturesExt();
  uint32_t count = bitcnt(f) + bitcnt(ef) + bitcnt(fe);
  return 17 * count;
}

void CPU::getFeaturesStr(char *buf, size_t bufsize) {
  uint64_t f = getFeatures(), ef = getExtFeatures(), fe = getFeaturesExt();
  char *end = buf + bufsize;

  for (int i = 0; i < 64; i++) {
    if (((f & (1ll << i)) != 0) && CPUID_FEAT_STR[i][0]) {
      buf += snprintf(buf, size_t(end - buf), "%s ", CPUID_FEAT_STR[i]);
    }
  }

  for (int i = 0; i < 64; i++) {
    if (((ef & (1ll << i)) != 0) && CPUID_EXT_FEAT_STR[i][0]) {
      buf += snprintf(buf, size_t(end - buf), "%s ", CPUID_EXT_FEAT_STR[i]);
    }
  }

  for (int i = 0; i < 64; i++) {
    if (((fe & (1ll << i)) != 0) && CPUID_FEAT_EXT_STR[i][0]) {
      buf += snprintf(buf, size_t(end - buf), "%s ", CPUID_FEAT_EXT_STR[i]);
    }
  }
  *buf = 0;
}

const char* CPU::getBrandString() {
  if ((brandString[0] == 0) && (getMaxCPUID() >= 0x80000004)) {
    uint32_t *v = brandString;
    uint32_t eax = 0x80000002;
    asm volatile("cpuid" : "+a"(eax), "=b"(v[1]), "=c"(v[2]), "=d"(v[3]));
    v[0] = eax; eax = 0x80000003;
    asm volatile("cpuid" : "+a"(eax), "=b"(v[5]), "=c"(v[6]), "=d"(v[7]));
    v[4] = eax; eax = 0x80000004;
    asm volatile("cpuid" : "+a"(eax), "=b"(v[9]), "=c"(v[10]), "=d"(v[11]));
    v[8] = eax;
    brandString[12] = 0;
  }
  return reinterpret_cast<const char*>(brandString);
}
