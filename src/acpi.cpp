//    PhoeniX OS ACPI subsystem
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

#include "acpi.hpp"
ACPI* ACPI::controller = 0;
unsigned char ACPI::activeCpuCount = 0;
_uint64 ACPI::busfreq = 0;
Mutex* ACPI::cpuCountMutex = 0;

ACPI* ACPI::getController() {
    if (controller) return controller;
    ACPI::cpuCountMutex = new Mutex();
    return controller = new ACPI();
}

ACPI::ACPI() {
    char *p = (char *)0x000e0000;
    char *end = (char *)0x000fffff;
    acpiCpuCount = 0;
    activeCpuCount = 1;
    
    if (!((CPU::getFeatures() >> 32) & CPUID_FEAT_APIC)) {
        acpiCpuCount = 1;
        return;
    }

    while (p < end)
    {
        _uint64 signature = *(_uint64 *)p;
        
        if ((signature == 0x2052545020445352) && ParseRsdp(p))
            break;
        
        p += 16;
    }
}
void ACPI::ParseDT(AcpiHeader *header) {
	Memory::salloc(header);
    
    if (header->signature == 0x43495041)
        ParseApic((AcpiMadt *)header);
}
void ACPI::ParseRsdt(AcpiHeader *rsdt) {
	Memory::salloc(rsdt);
    int *p = (int *)(rsdt + 1);
    int *end = (int *)((char*)rsdt + rsdt->length);
    
    while (p < end) {
        _uint64 address = (_uint64)((int)(*p++) & 0xFFFFFFFF);
        ParseDT((AcpiHeader *)(uintptr_t)address);
    }
}
void ACPI::ParseXsdt(AcpiHeader *xsdt) {
	Memory::salloc(xsdt);
    _uint64 *p = (_uint64 *)(xsdt + 1);
    _uint64 *end = (_uint64 *)((char*)xsdt + xsdt->length);
    
    while (p < end) {
        _uint64 address = *p++;
        ParseDT((AcpiHeader *)(uintptr_t)address);
    }
}
void ACPI::ParseApic(AcpiMadt *a_madt) {
	Memory::salloc(a_madt);
    madt = a_madt;
    
    localApicAddr = (long *)(uintptr_t)((_uint64)madt->localApicAddr & 0xFFFFFFFF);
	Memory::salloc(localApicAddr);
    
    char *p = (char *)(madt + 1);
    char *end = ((char *)madt)+madt->header.length;
	
    while (p < end) {
        ApicHeader *header = (ApicHeader *)p;
        short type = header->type;
        short length = header->length;
		Memory::salloc(header);
        if (type == 0) {
            ApicLocalApic *s = (ApicLocalApic *)p;
            if (acpiCpuCount < 256)
            {
                acpiCpuIds[acpiCpuCount] = s->apicId;
                ++acpiCpuCount;
            }
        } else if (type == 1) {
            ApicIoApic *s = (ApicIoApic *)p;
            ioApicAddr = (long *)(uintptr_t)((_uint64)s->ioApicAddress & 0xFFFFFFFF);
			Memory::salloc(ioApicAddr);
        } else if (type == 2) {
            ApicInterruptOverride *s = (ApicInterruptOverride *)p;
        }
        
        p += length;
    }
}
bool ACPI::ParseRsdp(char *p) {
    char sum = 0;
    for (int i = 0; i < 20; ++i)
        sum += p[i];
    
    if (sum)
        return false;
    
    char oem[7];
    Memory::copy(oem, (char*)(p + 9), 6);
    oem[6] = '\0';
    
    char revision = p[15];
    if (revision == 0) {
        int rsdtAddr = *(int *)(p + 16);
        ParseRsdt((AcpiHeader *)(uintptr_t)((_uint64)rsdtAddr & 0xFFFFFFFF));
    } else if (revision == 2) {
        int rsdtAddr = *(int *)(p + 16);
        _uint64 xsdtAddr = *(_uint64 *)(p + 24);
        
        if (xsdtAddr)
            ParseXsdt((AcpiHeader *)(uintptr_t)xsdtAddr);
        else
            ParseRsdt((AcpiHeader *)(uintptr_t)((_uint64)rsdtAddr & 0xFFFFFFFF));
    }
    
    return true;
}
int ACPI::getLapicID() {
    if (!localApicAddr) return 0;
    return LapicIn(LAPIC_APICID) >> 24;
}
int ACPI::getLapicIDOfCPU(int id) {
    if (!localApicAddr) return 0;
    return acpiCpuIds[id];
}
int ACPI::LapicIn(int reg) {
    if (!localApicAddr) return 0;
    Memory::salloc((char*)localApicAddr + reg);
    return MmioRead32((char*)localApicAddr + reg);
}
void ACPI::LapicOut(int reg, int data) {
    if (!localApicAddr) return;
    Memory::salloc((char*)localApicAddr + reg);
    MmioWrite32((char*)localApicAddr + reg, data);
}
void* ACPI::getLapicAddr() {
    return localApicAddr;
}
int ACPI::getCPUCount() {
    if (!localApicAddr) return 1;
    return acpiCpuCount;
}
int ACPI::getActiveCPUCount() {
    if (!localApicAddr) return 1;
    return activeCpuCount;
}
void ACPI::activateCPU() {
    if (!localApicAddr) return;
    Interrupts::loadVector();
    LapicOut(LAPIC_DFR,0xFFFFFFFF);
    LapicOut(LAPIC_LDR,(LapicIn(0xD0)&0x00FFFFFF)|1);
    LapicOut(LAPIC_LVT_TMR,LAPIC_DISABLE);
    LapicOut(LAPIC_LVT_PERF,4<<8);
    LapicOut(LAPIC_LVT_LINT0,LAPIC_DISABLE);
    LapicOut(LAPIC_LVT_LINT1,LAPIC_DISABLE);
    LapicOut(LAPIC_TASKPRIOR,0);
    
    asm("mov $0x1B,%ecx \n rdmsr \n bts $11,%eax \n wrmsr");
    
    LapicOut(LAPIC_SPURIOUS,0x27 | LAPIC_SW_ENABLE);
    
    _uint64 c = (ACPI::busfreq / 1000) >> 4;
    if (c < 0x10) c = 0x10;
    
    LapicOut(LAPIC_TMRINITCNT,c & 0xFFFFFFFF);
    LapicOut(LAPIC_LVT_TMR,0x20 | TMR_PERIODIC);
    LapicOut(LAPIC_TMRDIV,3);
    EOI();
    cpuCountMutex->lock();
    activeCpuCount++;
    cpuCountMutex->release();
}
void ACPI::sendCPUInit(int id) {
    if (!localApicAddr) return;
    LapicOut(LAPIC_ICRH, id << 24);
    LapicOut(LAPIC_ICRL, 0x00004500);
    while (LapicIn(LAPIC_ICRL) & 0x00001000);
}
void ACPI::sendCPUStartup(int id, char vector) {
    if (!localApicAddr) return;
    LapicOut(LAPIC_ICRH, id << 24);
    LapicOut(LAPIC_ICRL, vector | 0x00004600);
    while (LapicIn(LAPIC_ICRL) & 0x00001000);
}
void ACPI::EOI(){
    if (!(ACPI::getController())->localApicAddr) return;
    (ACPI::getController())->LapicOut(LAPIC_EOI,0);
}
bool ACPI::initAPIC(){
    if (!((CPU::getFeatures() >> 32) & CPUID_FEAT_APIC)) return false;
    if (localApicAddr == 0) return false;
    Interrupts::maskIRQ(0xFFFF);
    ACPI *acpi = (ACPI::getController());
    
    LapicOut(LAPIC_DFR,0xFFFFFFFF);
    LapicOut(LAPIC_LDR,(LapicIn(0xD0)&0x00FFFFFF)|1);
    LapicOut(LAPIC_LVT_TMR,LAPIC_DISABLE);
    LapicOut(LAPIC_LVT_PERF,4<<8);
    LapicOut(LAPIC_LVT_LINT0,LAPIC_DISABLE);
    LapicOut(LAPIC_LVT_LINT1,LAPIC_DISABLE);
    LapicOut(LAPIC_TASKPRIOR,0);
    
    asm("mov $0x1B,%ecx \n rdmsr \n bts $11,%eax \n wrmsr");

    LapicOut(LAPIC_SPURIOUS,0x27 | LAPIC_SW_ENABLE);
    
    outportb(0x61,inportb(0x61)&0xFD|1);
    outportb(0x43,0xB2);
    outportb(0x42,0x9B);
    inportb(0x60);
    outportb(0x42,0x2E);
    char t = inportb(0x61) & 0xFE;
    outportb(0x61,t);
    outportb(0x61,t|1);
    LapicOut(LAPIC_TMRINITCNT,-1);
    while (inportb(0x61) & 0x20 != 0) ;
    LapicOut(LAPIC_LVT_TMR,LAPIC_DISABLE);
    acpi->busfreq = ((-1 - LapicIn(0x390)) << 4) * 100;
    _uint64 c = (ACPI::busfreq / 1000) >> 4;
    if (c < 0x10) c = 0x10;
    
    LapicOut(LAPIC_TMRINITCNT,c & 0xFFFFFFFF);
    LapicOut(LAPIC_LVT_TMR,0x20 | TMR_PERIODIC);
    LapicOut(LAPIC_TMRDIV,3);
    EOI();
    return true;
}
