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

#include "cpu.hpp"

char CPU::vendor[13] = "";
_uint64 CPU::features = 0;

char* CPU::getVendor() {
    if (vendor[0]==0) {
        int eax=0, ebx, ecx, edx;
        asm("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(eax));
        *((int*)(&vendor[0])) = ebx;
        *((int*)(&vendor[4])) = edx;
        *((int*)(&vendor[8])) = ecx;
    }
    return &vendor[0];
}

_uint64 CPU::getFeatures(){
    if (features == 0) {
        int eax=1, ebx, ecx, edx;
        asm("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(eax));
        features = ecx;
        features <<= 32;
        features |= edx;
    }
    return features;
}

void __inline cf(char*d, const char*f){
    _uint64 p = 0, l = 0;
    while(d[p] != 0) p++;
    if (p != 0) d[p++] = ' ';
    while(f[l] != 0) {d[p+(l)] = f[l]; l++;}
    d[p+l]=0;
}

char* CPU::getFeaturesStr(){
    char* res;
    _uint64 f = CPU::getFeatures(), l = 0;

    if (f & CPUID_FEAT_SSE3) l += 5;
    if (f & CPUID_FEAT_PCLMUL) l += 7;
    if (f & CPUID_FEAT_DTES64) l += 7;
    if (f & CPUID_FEAT_MONITOR) l += 8;
    if (f & CPUID_FEAT_DS_CPL) l += 7;
    if (f & CPUID_FEAT_VMX) l += 4;
    if (f & CPUID_FEAT_SMX) l += 4;
    if (f & CPUID_FEAT_EST) l += 4;
    if (f & CPUID_FEAT_TM2) l += 4;
    if (f & CPUID_FEAT_SSSE3) l += 6;
    if (f & CPUID_FEAT_CID) l += 4;
    if (f & CPUID_FEAT_FMA) l += 4;
    if (f & CPUID_FEAT_CX16) l += 5;
    if (f & CPUID_FEAT_ETPRD) l += 6;
    if (f & CPUID_FEAT_PDCM) l += 5;
    if (f & CPUID_FEAT_DCA) l += 4;
    if (f & CPUID_FEAT_SSE4_1) l += 7;
    if (f & CPUID_FEAT_SSE4_2) l += 7;
    if (f & CPUID_FEAT_x2APIC) l += 7;
    if (f & CPUID_FEAT_MOVBE) l += 6;
    if (f & CPUID_FEAT_POPCNT) l += 7;
    if (f & CPUID_FEAT_AES) l += 4;
    if (f & CPUID_FEAT_XSAVE) l += 6;
    if (f & CPUID_FEAT_OSXSAVE) l += 8;
    if (f & CPUID_FEAT_AVX) l += 4;

    if ((f >> 32) & CPUID_FEAT_FPU) l += 4;
    if ((f >> 32) & CPUID_FEAT_VME) l += 4;
    if ((f >> 32) & CPUID_FEAT_DE) l += 3;
    if ((f >> 32) & CPUID_FEAT_PSE) l += 4;
    if ((f >> 32) & CPUID_FEAT_TSC) l += 4;
    if ((f >> 32) & CPUID_FEAT_MSR) l += 4;
    if ((f >> 32) & CPUID_FEAT_PAE) l += 4;
    if ((f >> 32) & CPUID_FEAT_MCE) l += 4;
    if ((f >> 32) & CPUID_FEAT_CX8) l += 4;
    if ((f >> 32) & CPUID_FEAT_APIC) l += 5;
    if ((f >> 32) & CPUID_FEAT_SEP) l += 4;
    if ((f >> 32) & CPUID_FEAT_MTRR) l += 5;
    if ((f >> 32) & CPUID_FEAT_PGE) l += 4;
    if ((f >> 32) & CPUID_FEAT_MCA) l += 4;
    if ((f >> 32) & CPUID_FEAT_CMOV) l += 5;
    if ((f >> 32) & CPUID_FEAT_PAT) l += 4;
    if ((f >> 32) & CPUID_FEAT_PSE36) l += 6;
    if ((f >> 32) & CPUID_FEAT_PSN) l += 4;
    if ((f >> 32) & CPUID_FEAT_CLF) l += 4;
    if ((f >> 32) & CPUID_FEAT_DTES) l += 5;
    if ((f >> 32) & CPUID_FEAT_ACPI) l += 5;
    if ((f >> 32) & CPUID_FEAT_MMX) l += 4;
    if ((f >> 32) & CPUID_FEAT_FXSR) l += 5;
    if ((f >> 32) & CPUID_FEAT_SSE) l += 4;
    if ((f >> 32) & CPUID_FEAT_SSE2) l += 5;
    if ((f >> 32) & CPUID_FEAT_SS) l += 3;
    if ((f >> 32) & CPUID_FEAT_HTT) l += 4;
    if ((f >> 32) & CPUID_FEAT_TM1) l += 4;
    if ((f >> 32) & CPUID_FEAT_IA64) l += 5;
    if ((f >> 32) & CPUID_FEAT_PBE) l += 4;
    
    res = (char*)Memory::alloc(l);
    
    if (f & CPUID_FEAT_SSE3) cf(res,"sse3");
    if (f & CPUID_FEAT_PCLMUL) cf(res,"pclmul");
    if (f & CPUID_FEAT_DTES64) cf(res,"dtes64");
    if (f & CPUID_FEAT_MONITOR) cf(res,"monitor");
    if (f & CPUID_FEAT_DS_CPL) cf(res,"ds_cpl");
    if (f & CPUID_FEAT_VMX) cf(res,"vmx");
    if (f & CPUID_FEAT_SMX) cf(res,"smx");
    if (f & CPUID_FEAT_EST) cf(res,"est");
    if (f & CPUID_FEAT_TM2) cf(res,"tm2");
    if (f & CPUID_FEAT_SSSE3) cf(res,"ssse3");
    if (f & CPUID_FEAT_CID) cf(res,"cid");
    if (f & CPUID_FEAT_FMA) cf(res,"fma");
    if (f & CPUID_FEAT_CX16) cf(res,"cx16");
    if (f & CPUID_FEAT_ETPRD) cf(res,"etprd");
    if (f & CPUID_FEAT_PDCM) cf(res,"pdcm");
    if (f & CPUID_FEAT_DCA) cf(res,"dca");
    if (f & CPUID_FEAT_SSE4_1) cf(res,"sse4_1");
    if (f & CPUID_FEAT_SSE4_2) cf(res,"sse4_2");
    if (f & CPUID_FEAT_x2APIC) cf(res,"x2apic");
    if (f & CPUID_FEAT_MOVBE) cf(res,"movbe");
    if (f & CPUID_FEAT_POPCNT) cf(res,"popcnt");
    if (f & CPUID_FEAT_AES) cf(res,"aes");
    if (f & CPUID_FEAT_XSAVE) cf(res,"xsave");
    if (f & CPUID_FEAT_OSXSAVE) cf(res,"osxsave");
    if (f & CPUID_FEAT_AVX) cf(res,"avx");
    
    if ((f >> 32) & CPUID_FEAT_FPU) cf(res,"fpu");
    if ((f >> 32) & CPUID_FEAT_VME) cf(res,"vme");
    if ((f >> 32) & CPUID_FEAT_DE) cf(res,"de");
    if ((f >> 32) & CPUID_FEAT_PSE) cf(res,"pse");
    if ((f >> 32) & CPUID_FEAT_TSC) cf(res,"tsc");
    if ((f >> 32) & CPUID_FEAT_MSR) cf(res,"msr");
    if ((f >> 32) & CPUID_FEAT_PAE) cf(res,"pae");
    if ((f >> 32) & CPUID_FEAT_MCE) cf(res,"mce");
    if ((f >> 32) & CPUID_FEAT_CX8) cf(res,"cx8");
    if ((f >> 32) & CPUID_FEAT_APIC) cf(res,"apic");
    if ((f >> 32) & CPUID_FEAT_SEP) cf(res,"sep");
    if ((f >> 32) & CPUID_FEAT_MTRR) cf(res,"mtrr");
    if ((f >> 32) & CPUID_FEAT_PGE) cf(res,"pge");
    if ((f >> 32) & CPUID_FEAT_MCA) cf(res,"mca");
    if ((f >> 32) & CPUID_FEAT_CMOV) cf(res,"cmov");
    if ((f >> 32) & CPUID_FEAT_PAT) cf(res,"pat");
    if ((f >> 32) & CPUID_FEAT_PSE36) cf(res,"pse36");
    if ((f >> 32) & CPUID_FEAT_PSN) cf(res,"psn");
    if ((f >> 32) & CPUID_FEAT_CLF) cf(res,"clf");
    if ((f >> 32) & CPUID_FEAT_DTES) cf(res,"dtes");
    if ((f >> 32) & CPUID_FEAT_ACPI) cf(res,"acpi");
    if ((f >> 32) & CPUID_FEAT_MMX) cf(res,"mmx");
    if ((f >> 32) & CPUID_FEAT_FXSR) cf(res,"fxsr");
    if ((f >> 32) & CPUID_FEAT_SSE) cf(res,"sse");
    if ((f >> 32) & CPUID_FEAT_SSE2) cf(res,"sse2");
    if ((f >> 32) & CPUID_FEAT_SS) cf(res,"ss");
    if ((f >> 32) & CPUID_FEAT_HTT) cf(res,"htt");
    if ((f >> 32) & CPUID_FEAT_TM1) cf(res,"tm1");
    if ((f >> 32) & CPUID_FEAT_IA64) cf(res,"ia64");
    if ((f >> 32) & CPUID_FEAT_PBE) cf(res,"pbe");
    
    return res;
}