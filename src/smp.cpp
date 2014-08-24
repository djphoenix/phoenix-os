//    PhoeniX OS SMP Subsystem
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

#include "smp.hpp"
unsigned char g_acpiCpuCount = 0;
unsigned char g_activeCpuCount = 0;
int g_acpiCpuIds[256];
long* g_localApicAddr, *g_ioApicAddr;
AcpiMadt *s_madt;

inline void MmioWrite32(void *p, int data) {
	salloc(p);
    *(volatile int *)(p) = data;
}
inline int MmioRead32(void *p) {
	salloc(p);
    return *(volatile int *)(p);
}
static int LocalApicIn(int reg) {
    return MmioRead32((char*)g_localApicAddr + reg);
}
static void LocalApicOut(int reg, int data) {
    MmioWrite32((char*)g_localApicAddr + reg, data);
}

void AcpiParseApic(AcpiMadt *madt) {
	salloc(madt);
    s_madt = madt;

    g_localApicAddr = (long *)(uintptr_t)((_uint64)madt->localApicAddr & 0xFFFFFFFF);
	salloc(g_localApicAddr);

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
		salloc(header);
        if (type == 0) {
            ApicLocalApic *s = (ApicLocalApic *)p;

/*            print("Found CPU: ");
			printl(s->acpiProcessorId);
			print(" ");
			printl(s->apicId);
			print(" ");
			prints(s->flags);
			print("\n");*/
            if (g_acpiCpuCount < 256)
            {
                g_acpiCpuIds[g_acpiCpuCount] = s->apicId;
                ++g_acpiCpuCount;
            }
        } else if (type == 1) {
            ApicIoApic *s = (ApicIoApic *)p;
            g_ioApicAddr = (long *)(uintptr_t)((_uint64)s->ioApicAddress & 0xFFFFFFFF);
			salloc(g_ioApicAddr);
        } else if (type == 2) {
            ApicInterruptOverride *s = (ApicInterruptOverride *)p;
        }

        p += length;
    }
}


static void AcpiParseDT(AcpiHeader *header) {
	salloc(header);

    if (header->signature == 0x43495041)
        AcpiParseApic((AcpiMadt *)header);
}

void AcpiParseRsdt(AcpiHeader *rsdt) {
	salloc(rsdt);
    int *p = (int *)(rsdt + 1);
    int *end = (int *)((char*)rsdt + rsdt->length);

    while (p < end) {
        _uint64 address = (_uint64)((int)(*p++) & 0xFFFFFFFF);
        AcpiParseDT((AcpiHeader *)(uintptr_t)address);
    }
}

void AcpiParseXsdt(AcpiHeader *xsdt) {
	salloc(xsdt);
    _uint64 *p = (_uint64 *)(xsdt + 1);
    _uint64 *end = (_uint64 *)((char*)xsdt + xsdt->length);

    while (p < end) {
        _uint64 address = *p++;
        AcpiParseDT((AcpiHeader *)(uintptr_t)address);
    }
}

bool AcpiParseRsdp(char *p) {
    char sum = 0;
    for (int i = 0; i < 20; ++i)
        sum += p[i];

    if (sum)
        return false;

    char oem[7];
    memcpy(oem, (char*)(p + 9), 6);
    oem[6] = '\0';

    char revision = p[15];
    if (revision == 0) {
        int rsdtAddr = *(int *)(p + 16);
        AcpiParseRsdt((AcpiHeader *)(uintptr_t)((_uint64)rsdtAddr & 0xFFFFFFFF));
    } else if (revision == 2) {
        int rsdtAddr = *(int *)(p + 16);
        _uint64 xsdtAddr = *(_uint64 *)(p + 24);

        if (xsdtAddr)
            AcpiParseXsdt((AcpiHeader *)(uintptr_t)xsdtAddr);
        else
            AcpiParseRsdt((AcpiHeader *)(uintptr_t)((_uint64)rsdtAddr & 0xFFFFFFFF));
    }

    return true;
}
void AcpiInit() {
    char *p = (char *)0x000e0000;
    char *end = (char *)0x000fffff;

    while (p < end)
    {
        _uint64 signature = *(_uint64 *)p;

        if ((signature == 0x2052545020445352) && AcpiParseRsdp(p))
            break;

        p += 16;
    }
}
void LocalApicBroadcastInit(int id) {
    LocalApicOut(0x0310, id << 24);
    LocalApicOut(0x0300, 0x000C4500);

    while (LocalApicIn(0x0300) & 0x00001000)
        ;
}
void LocalApicBroadcastStartup(int id, char vector) {
    LocalApicOut(0x0310, id << 24);
    LocalApicOut(0x0300, vector | 0x000C4600);
	
    while (LocalApicIn(0x0300) & 0x00001000)
        ;
}
void LocalApicSendInit(int id) {
    LocalApicOut(0x0310, id << 24);
    LocalApicOut(0x0300, 0x00004500);

    while (LocalApicIn(0x0300) & 0x00001000)
        ;
}
void LocalApicSendStartup(int id, char vector) {
    LocalApicOut(0x0310, id << 24);
    LocalApicOut(0x0300, vector | 0x00004600);
	
    while (LocalApicIn(0x0300) & 0x00001000)
        ;
}
int LocalApicGetId() {
    return LocalApicIn(0x0020) >> 24;
}

void smp_start() {
	g_activeCpuCount++;
	process_loop();
}

void smp_init() {
	AcpiInit();
	g_activeCpuCount = 1;
	int localId = LocalApicGetId();	
	char* smp_init_code = (char*)palloc(1);
	char smp_init_vector = (((_uint64)smp_init_code) >> 12) & 0xFF;
	char* smp_s; asm("mov $_smp_init,%q0":"=a"(smp_s):);
	char* smp_e; asm("mov $_smp_end,%q0":"=a"(smp_e):);
	while(smp_s < smp_e) *(smp_init_code++) = *(smp_s++);
	*((_uint64*)smp_init_code) = (_uint64)g_localApicAddr; smp_init_code += 8;
	_uint64 *stacks = (_uint64*)malloc(sizeof(_uint64)*g_acpiCpuCount);
	for(int i = 0; i < g_acpiCpuCount; i++)
		if(g_acpiCpuIds[i] != localId)
			stacks[i] = ((_uint64)palloc(1))+0x1000;
	*((_uint64*)smp_init_code) = (_uint64)stacks; smp_init_code += 8;
	*((_uint64*)smp_init_code) = (_uint64)(&smp_start);
	for(int i = 0; i < g_acpiCpuCount; i++)
		if(g_acpiCpuIds[i] != localId)
			LocalApicSendInit(g_acpiCpuIds[i]);
	asm("hlt");
	for(int i = 0; i < g_acpiCpuCount; i++)
		if(g_acpiCpuIds[i] != localId)
			LocalApicSendStartup(g_acpiCpuIds[i],smp_init_vector);
	asm("hlt");
	for(int i = 0; i < g_acpiCpuCount; i++)
		if(g_acpiCpuIds[i] != localId)
			LocalApicSendStartup(g_acpiCpuIds[i],smp_init_vector);

	while (g_activeCpuCount != g_acpiCpuCount) asm("hlt");

	mfree(stacks);
}