//    PhoeniX OS CPUID subsystem
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

#ifndef CPU_H
#define CPU_H

#include "pxlib.hpp"
#include "memory.hpp"

enum {
    CPUID_FEAT_SSE3         = 1 << 0,
    CPUID_FEAT_PCLMUL       = 1 << 1,
    CPUID_FEAT_DTES64       = 1 << 2,
    CPUID_FEAT_MONITOR      = 1 << 3,
    CPUID_FEAT_DS_CPL       = 1 << 4,
    CPUID_FEAT_VMX          = 1 << 5,
    CPUID_FEAT_SMX          = 1 << 6,
    CPUID_FEAT_EST          = 1 << 7,
    CPUID_FEAT_TM2          = 1 << 8,
    CPUID_FEAT_SSSE3        = 1 << 9,
    CPUID_FEAT_CID          = 1 << 10,
    CPUID_FEAT_FMA          = 1 << 12,
    CPUID_FEAT_CX16         = 1 << 13,
    CPUID_FEAT_ETPRD        = 1 << 14,
    CPUID_FEAT_PDCM         = 1 << 15,
    CPUID_FEAT_DCA          = 1 << 18,
    CPUID_FEAT_SSE4_1       = 1 << 19,
    CPUID_FEAT_SSE4_2       = 1 << 20,
    CPUID_FEAT_x2APIC       = 1 << 21,
    CPUID_FEAT_MOVBE        = 1 << 22,
    CPUID_FEAT_POPCNT       = 1 << 23,
    CPUID_FEAT_AES          = 1 << 25,
    CPUID_FEAT_XSAVE        = 1 << 26,
    CPUID_FEAT_OSXSAVE      = 1 << 27,
    CPUID_FEAT_AVX          = 1 << 28,
    
    CPUID_FEAT_FPU          = 1 << 0,
    CPUID_FEAT_VME          = 1 << 1,
    CPUID_FEAT_DE           = 1 << 2,
    CPUID_FEAT_PSE          = 1 << 3,
    CPUID_FEAT_TSC          = 1 << 4,
    CPUID_FEAT_MSR          = 1 << 5,
    CPUID_FEAT_PAE          = 1 << 6,
    CPUID_FEAT_MCE          = 1 << 7,
    CPUID_FEAT_CX8          = 1 << 8,
    CPUID_FEAT_APIC         = 1 << 9,
    CPUID_FEAT_SEP          = 1 << 11,
    CPUID_FEAT_MTRR         = 1 << 12,
    CPUID_FEAT_PGE          = 1 << 13,
    CPUID_FEAT_MCA          = 1 << 14,
    CPUID_FEAT_CMOV         = 1 << 15,
    CPUID_FEAT_PAT          = 1 << 16,
    CPUID_FEAT_PSE36        = 1 << 17,
    CPUID_FEAT_PSN          = 1 << 18,
    CPUID_FEAT_CLF          = 1 << 19,
    CPUID_FEAT_DTES         = 1 << 21,
    CPUID_FEAT_ACPI         = 1 << 22,
    CPUID_FEAT_MMX          = 1 << 23,
    CPUID_FEAT_FXSR         = 1 << 24,
    CPUID_FEAT_SSE          = 1 << 25,
    CPUID_FEAT_SSE2         = 1 << 26,
    CPUID_FEAT_SS           = 1 << 27,
    CPUID_FEAT_HTT          = 1 << 28,
    CPUID_FEAT_TM1          = 1 << 29,
    CPUID_FEAT_IA64         = 1 << 30,
    CPUID_FEAT_PBE          = 1 << 31
};

class CPU {
private:
    static char vendor[13];
    static _uint64 features;
public:
    static char* getVendor();
    static _uint64 getFeatures();
    static char* getFeaturesStr();
};

#endif