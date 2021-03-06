//    PhoeniX OS CPUID subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once

#include <stdint.h>
#include <stddef.h>

class CPU {
 public:
  enum Feature: uint64_t {
    CPUID_FEAT_FPU /*    */ = 0x0000000000000001ULL,
    CPUID_FEAT_VME /*    */ = 0x0000000000000002ULL,
    CPUID_FEAT_DE /*     */ = 0x0000000000000004ULL,
    CPUID_FEAT_PSE /*    */ = 0x0000000000000008ULL,
    CPUID_FEAT_TSC /*    */ = 0x0000000000000010ULL,
    CPUID_FEAT_MSR /*    */ = 0x0000000000000020ULL,
    CPUID_FEAT_PAE /*    */ = 0x0000000000000040ULL,
    CPUID_FEAT_MCE /*    */ = 0x0000000000000080ULL,
    CPUID_FEAT_CX8 /*    */ = 0x0000000000000100ULL,
    CPUID_FEAT_APIC /*   */ = 0x0000000000000200ULL,
    /* UNK                  = 0x0000000000000400ULL */
    CPUID_FEAT_SEP /*    */ = 0x0000000000000800ULL,
    CPUID_FEAT_MTRR /*   */ = 0x0000000000001000ULL,
    CPUID_FEAT_PGE /*    */ = 0x0000000000002000ULL,
    CPUID_FEAT_MCA /*    */ = 0x0000000000004000ULL,
    CPUID_FEAT_CMOV /*   */ = 0x0000000000008000ULL,
    CPUID_FEAT_PAT /*    */ = 0x0000000000010000ULL,
    CPUID_FEAT_PSE36 /*  */ = 0x0000000000020000ULL,
    CPUID_FEAT_PSN /*    */ = 0x0000000000040000ULL,
    CPUID_FEAT_CLFSH /*  */ = 0x0000000000080000ULL,
    /* UNK                  = 0x0000000000100000ULL */
    CPUID_FEAT_DS /*     */ = 0x0000000000200000ULL,
    CPUID_FEAT_ACPI /*   */ = 0x0000000000400000ULL,
    CPUID_FEAT_MMX /*    */ = 0x0000000000800000ULL,
    CPUID_FEAT_FXSR /*   */ = 0x0000000001000000ULL,
    CPUID_FEAT_SSE /*    */ = 0x0000000002000000ULL,
    CPUID_FEAT_SSE2 /*   */ = 0x0000000004000000ULL,
    CPUID_FEAT_SS /*     */ = 0x0000000008000000ULL,
    CPUID_FEAT_HTT /*    */ = 0x0000000010000000ULL,
    CPUID_FEAT_TM /*     */ = 0x0000000020000000ULL,
    CPUID_FEAT_IA64 /*   */ = 0x0000000040000000ULL,
    CPUID_FEAT_PBE /*    */ = 0x0000000080000000ULL,
    CPUID_FEAT_SSE3 /*   */ = 0x0000000100000000ULL,
    CPUID_FEAT_PCLMUL /* */ = 0x0000000200000000ULL,
    CPUID_FEAT_DTES64 /* */ = 0x0000000400000000ULL,
    CPUID_FEAT_MONITOR /**/ = 0x0000000800000000ULL,
    CPUID_FEAT_DS_CPL /* */ = 0x0000001000000000ULL,
    CPUID_FEAT_VMX /*    */ = 0x0000002000000000ULL,
    CPUID_FEAT_SMX /*    */ = 0x0000004000000000ULL,
    CPUID_FEAT_EST /*    */ = 0x0000008000000000ULL,
    CPUID_FEAT_TM2 /*    */ = 0x0000010000000000ULL,
    CPUID_FEAT_SSSE3 /*  */ = 0x0000020000000000ULL,
    CPUID_FEAT_CNTX_ID /**/ = 0x0000040000000000ULL,
    CPUID_FEAT_SDBG /*   */ = 0x0000080000000000ULL,
    CPUID_FEAT_FMA /*    */ = 0x0000100000000000ULL,
    CPUID_FEAT_CX16 /*   */ = 0x0000200000000000ULL,
    CPUID_FEAT_XTPR /*   */ = 0x0000400000000000ULL,
    CPUID_FEAT_PDCM /*   */ = 0x0000800000000000ULL,
    /* UNK                  = 0x0001000000000000ULL */
    CPUID_FEAT_PCID /*   */ = 0x0002000000000000ULL,
    CPUID_FEAT_DCA /*    */ = 0x0004800000000000ULL,
    CPUID_FEAT_SSE4_1 /* */ = 0x0008000000000000ULL,
    CPUID_FEAT_SSE4_2 /* */ = 0x0010000000000000ULL,
    CPUID_FEAT_x2APIC /* */ = 0x0020000000000000ULL,
    CPUID_FEAT_MOVBE /*  */ = 0x0040000000000000ULL,
    CPUID_FEAT_POPCNT /* */ = 0x0080000000000000ULL,
    CPUID_FEAT_TSC_DL /* */ = 0x0100000000000000ULL,
    CPUID_FEAT_AES /*    */ = 0x0200000000000000ULL,
    CPUID_FEAT_XSAVE /*  */ = 0x0400000000000000ULL,
    CPUID_FEAT_OSXSAVE /**/ = 0x0800000000000000ULL,
    CPUID_FEAT_AVX /*    */ = 0x1000000000000000ULL,
    CPUID_FEAT_F16C /*   */ = 0x2000000000000000ULL,
    CPUID_FEAT_RDRND /*  */ = 0x4000000000000000ULL,
    CPUID_FEAT_HV /*     */ = 0x8000000000000000ULL,
  };

  enum FeatureExt: uint64_t {
    CPUID_FEAT_EXT_FSGSB /*  */ = 0x0000000000000001ULL,
    CPUID_FEAT_EXT_TSCADJ /* */ = 0x0000000000000002ULL,
    CPUID_FEAT_EXT_SGX /*    */ = 0x0000000000000004ULL,
    CPUID_FEAT_EXT_BMI1 /*   */ = 0x0000000000000008ULL,
    CPUID_FEAT_EXT_HLE /*    */ = 0x0000000000000010ULL,
    CPUID_FEAT_EXT_AVX2 /*   */ = 0x0000000000000020ULL,
    /* UNK                      = 0x0000000000000040ULL */
    CPUID_FEAT_EXT_SMEP /*   */ = 0x0000000000000080ULL,
    CPUID_FEAT_EXT_BMI2 /*   */ = 0x0000000000000100ULL,
    CPUID_FEAT_EXT_ERMS /*   */ = 0x0000000000000200ULL,
    CPUID_FEAT_EXT_INVPCID /**/ = 0x0000000000000400ULL,
    CPUID_FEAT_EXT_RTM /*    */ = 0x0000000000000800ULL,
    CPUID_FEAT_EXT_PQM /*    */ = 0x0000000000001000ULL,
    CPUID_FEAT_EXT_FCSFDS /* */ = 0x0000000000002000ULL,
    CPUID_FEAT_EXT_MPX /*    */ = 0x0000000000004000ULL,
    CPUID_FEAT_EXT_PQE /*    */ = 0x0000000000008000ULL,
    CPUID_FEAT_EXT_A512F /*  */ = 0x0000000000010000ULL,
    CPUID_FEAT_EXT_A512DQ /* */ = 0x0000000000020000ULL,
    CPUID_FEAT_EXT_RDSEED /* */ = 0x0000000000040000ULL,
    CPUID_FEAT_EXT_ADX /*    */ = 0x0000000000080000ULL,
    CPUID_FEAT_EXT_SMAP /*   */ = 0x0000000000100000ULL,
    CPUID_FEAT_EXT_A512IMA /**/ = 0x0000000000200000ULL,
    CPUID_FEAT_EXT_PCOMMIT /**/ = 0x0000000000400000ULL,
    CPUID_FEAT_EXT_CLFOPT /* */ = 0x0000000000800000ULL,
    CPUID_FEAT_EXT_CLWB /*   */ = 0x0000000001000000ULL,
    CPUID_FEAT_EXT_INTELPT /**/ = 0x0000000002000000ULL,
    CPUID_FEAT_EXT_A512PF /* */ = 0x0000000004000000ULL,
    CPUID_FEAT_EXT_A512ER /* */ = 0x0000000008000000ULL,
    CPUID_FEAT_EXT_A512CD /* */ = 0x0000000010000000ULL,
    CPUID_FEAT_EXT_SHA /*    */ = 0x0000000020000000ULL,
    CPUID_FEAT_EXT_A512BW /* */ = 0x0000000040000000ULL,
    CPUID_FEAT_EXT_A512VL /* */ = 0x0000000080000000ULL,
    CPUID_FEAT_EXT_PREWT1 /* */ = 0x0000000100000000ULL,
    CPUID_FEAT_EXT_A512VBM /**/ = 0x0000000200000000ULL,
    /* UNK                      = 0x0000000400000000ULL, */
    CPUID_FEAT_EXT_PKU /*    */ = 0x0000000800000000ULL,
    CPUID_FEAT_EXT_OSPKE /*  */ = 0x0000001000000000ULL,
  };

  enum ExtFeature: uint64_t {
    CPUID_EXT_FEAT_FPU /*    */ = 0x0000000000000001ULL,
    CPUID_EXT_FEAT_VME /*    */ = 0x0000000000000002ULL,
    CPUID_EXT_FEAT_DE /*     */ = 0x0000000000000004ULL,
    CPUID_EXT_FEAT_PSE /*    */ = 0x0000000000000008ULL,
    CPUID_EXT_FEAT_TSC /*    */ = 0x0000000000000010ULL,
    CPUID_EXT_FEAT_MSR /*    */ = 0x0000000000000020ULL,
    CPUID_EXT_FEAT_PAE /*    */ = 0x0000000000000040ULL,
    CPUID_EXT_FEAT_MCE /*    */ = 0x0000000000000080ULL,
    CPUID_EXT_FEAT_CX8 /*    */ = 0x0000000000000100ULL,
    CPUID_EXT_FEAT_APIC /*   */ = 0x0000000000000200ULL,
    /* UNK                      = 0x0000000000000400ULL */
    CPUID_EXT_FEAT_SEP /*    */ = 0x0000000000000800ULL,
    CPUID_EXT_FEAT_MTRR /*   */ = 0x0000000000001000ULL,
    CPUID_EXT_FEAT_PGE /*    */ = 0x0000000000002000ULL,
    CPUID_EXT_FEAT_MCA /*    */ = 0x0000000000004000ULL,
    CPUID_EXT_FEAT_CMOV /*   */ = 0x0000000000008000ULL,
    CPUID_EXT_FEAT_PAT /*    */ = 0x0000000000010000ULL,
    CPUID_EXT_FEAT_PSE36 /*  */ = 0x0000000000020000ULL,
    /* UNK                      = 0x0000000000040000ULL */
    CPUID_EXT_FEAT_MP /*     */ = 0x0000000000080000ULL,
    CPUID_EXT_FEAT_NX /*     */ = 0x0000000000100000ULL,
    /* UNK                      = 0x0000000000200000ULL */
    CPUID_EXT_FEAT_MMXEXT /* */ = 0x0000000000400000ULL,
    CPUID_EXT_FEAT_MMX /*    */ = 0x0000000000800000ULL,
    CPUID_EXT_FEAT_FXSR /*   */ = 0x0000000001000000ULL,
    CPUID_EXT_FEAT_FXSROPT /**/ = 0x0000000002000000ULL,
    CPUID_EXT_FEAT_PDPE1GB /**/ = 0x0000000004000000ULL,
    CPUID_EXT_FEAT_RDTSCP /* */ = 0x0000000008000000ULL,
    /* UNK                      = 0x0000000010000000ULL */
    CPUID_EXT_FEAT_LM /*     */ = 0x0000000020000000ULL,
    CPUID_EXT_FEAT_3DNOWE /* */ = 0x0000000040000000ULL,
    CPUID_EXT_FEAT_3DNOW /*  */ = 0x0000000080000000ULL,
    CPUID_EXT_FEAT_LAHFLM /* */ = 0x0000000100000000ULL,
    CPUID_EXT_FEAT_CMPLEG /* */ = 0x0000000200000000ULL,
    CPUID_EXT_FEAT_SVM /*    */ = 0x0000000400000000ULL,
    CPUID_EXT_FEAT_EXTAPIC /**/ = 0x0000000800000000ULL,
    CPUID_EXT_FEAT_CR8LEG /* */ = 0x0000001000000000ULL,
    CPUID_EXT_FEAT_ABM /*    */ = 0x0000002000000000ULL,
    CPUID_EXT_FEAT_SSE4A /*  */ = 0x0000004000000000ULL,
    CPUID_EXT_FEAT_MASSE /*  */ = 0x0000008000000000ULL,
    CPUID_EXT_FEAT_3DNOWPF /**/ = 0x0000010000000000ULL,
    CPUID_EXT_FEAT_OSVW /*   */ = 0x0000020000000000ULL,
    CPUID_EXT_FEAT_IBS /*    */ = 0x0000040000000000ULL,
    CPUID_EXT_FEAT_XOP /*    */ = 0x0000080000000000ULL,
    CPUID_EXT_FEAT_SKINIT /* */ = 0x0000100000000000ULL,
    CPUID_EXT_FEAT_WDT /*    */ = 0x0000200000000000ULL,
    /* UNK                      = 0x0000400000000000ULL */
    CPUID_EXT_FEAT_LWP /*    */ = 0x0000800000000000ULL,
    CPUID_EXT_FEAT_FMA4 /*   */ = 0x0001000000000000ULL,
    CPUID_EXT_FEAT_TCE /*    */ = 0x0002000000000000ULL,
    /* UNK                      = 0x0004800000000000ULL */
    CPUID_EXT_FEAT_NODEID /* */ = 0x0008000000000000ULL,
    /* UNK                      = 0x0010000000000000ULL */
    CPUID_EXT_FEAT_TBM /*    */ = 0x0020000000000000ULL,
    CPUID_EXT_FEAT_TOPOEXT /**/ = 0x0040000000000000ULL,
    CPUID_EXT_FEAT_PCTRCRE /**/ = 0x0080000000000000ULL,
    CPUID_EXT_FEAT_PCTRNB /* */ = 0x0100000000000000ULL,
    /* UNK                      = 0x0200000000000000ULL */
    CPUID_EXT_FEAT_DBX /*    */ = 0x0400000000000000ULL,
    CPUID_EXT_FEAT_PERFTSC /**/ = 0x0800000000000000ULL,
    CPUID_EXT_FEAT_PCXL2I /* */ = 0x1000000000000000ULL,
    /* UNK                      = 0x2000000000000000ULL */
    /* UNK                      = 0x4000000000000000ULL */
    /* UNK                      = 0x8000000000000000ULL */
  };

  struct Info {
    uint32_t stepping, type, model, family;
  };

 private:
  static uint64_t features;
  static uint64_t features_ext;
  static uint64_t ext_features;
  static struct Info info;
  static uint32_t maxCPUID;

  static uint32_t vendor[4];
  static uint32_t brandString[13];

 public:
  static const char* getVendor();
  static uint64_t getFeatures();
  static uint64_t getFeaturesExt();
  static uint64_t getExtFeatures();
  static uint64_t getFeaturesStrSize();
  static void getFeaturesStr(char *buf, size_t bufsize);
  static struct Info getInfo();
  static uint32_t getMaxCPUID();
  static const char* getBrandString();
};
