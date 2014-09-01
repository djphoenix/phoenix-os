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
#define quantum 1000

ACPI* ACPI::getController() {
    if (controller) return controller;
    return controller = new ACPI();
}

ACPI::ACPI() {
    char *p = (char *)0x000e0000;
    char *end = (char *)0x000fffff;
    acpiCpuCount = 0;
    activeCpuCount = 1;
    
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
    
    //    LocalApicOut(0x0080, 0);
    //    LocalApicOut(0x00e0, 0xffffffff);
    //    LocalApicOut(0x00d0, 0x01000000);
    //    LocalApicOut(0x00f0, 0x100 | 0xff);
	
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
    return LapicIn(0x0020) >> 24;
}
int ACPI::getLapicIDOfCPU(int id) {
    return acpiCpuIds[id];
}
int ACPI::LapicIn(int reg) {
    return MmioRead32((char*)localApicAddr + reg);
}
void ACPI::LapicOut(int reg, int data) {
    MmioWrite32((char*)localApicAddr + reg, data);
}
void* ACPI::getLapicAddr() {
    return localApicAddr;
}
int ACPI::getCPUCount() {
    return acpiCpuCount;
}
int ACPI::getActiveCPUCount() {
    return activeCpuCount;
}
void ACPI::activateCPU() {
    activeCpuCount++;
}
void ACPI::sendCPUInit(int id) {
    LapicOut(0x0310, id << 24);
    LapicOut(0x0300, 0x00004500);
    while (LapicIn(0x0300) & 0x00001000);
}
void ACPI::sendCPUStartup(int id, char vector) {
    LapicOut(0x0310, id << 24);
    LapicOut(0x0300, vector | 0x00004600);
    while (LapicIn(0x0300) & 0x00001000);
}
void ACPI::EOI(){
    (ACPI::getController())->LapicOut(0xB0,0);
}
void ACPI::initTimer(){
    if (localApicAddr == 0) return;
    LapicOut(0xE0,0xFFFFFFFF);
    LapicOut(0xD0,(LapicIn(0xD0)&0x00FFFFFF)|1);
    LapicOut(0x320,0x10000);
    LapicOut(0x340,4<<8);
    LapicOut(0x350,0x10000);
    LapicOut(0x360,0x10000);
    LapicOut(0x80,0);
    
    unsigned int base;
    asm("rdmsr":"=A"(base):"c"(0x1B));
    base &= 0xfffff100;
    asm("wrmsr"::"c"(0x1B),"A"(base|0x800));
    
    LapicOut(0xF0,LapicIn(0xF0) | 0x100);
    LapicOut(0x320,0x20);
    LapicOut(0x3E0,3);

    outportb(0x61,(inportb(0x61)&0xFD)|1);
	outportb(0x43,/*0xB2*/0);
	outportb(0x42,0x9B);
	inportb(0x60);
	outportb(0x42,0x2E);

	unsigned char tmp=inportb(0x61)&0xFE;
	outportb(0x61,tmp);
	outportb(0x61,tmp|1);
    LapicOut(0x380,0xFFFFFFFF);
    char t;
	while(!((t=inportb(0x61))&0x20)) {
        printb(t);
    };
    
    LapicOut(0x320,0x10000);

	unsigned int cpubusfreq = ((0xFFFFFFFF-LapicIn(0x390))+1)*16*100, tmp2 = cpubusfreq/quantum/16;
    
    Interrupts::maskIRQ(Interrupts::getIRQmask()|2);
    
    LapicOut(0x380,tmp2<16?16:tmp2);
    LapicOut(0x320,0x20|0x20000);
    LapicOut(0x3E0,3);
    Interrupts::addCallback(0x20,&ACPI::EOI);
}
