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
uint8_t ACPI::activeCpuCount = 0;
uint64_t ACPI::busfreq = 0;

static const char* ACPI_FIND_START = (const char*)0x000e0000;
static const char* ACPI_FIND_TOP = (const char*)0x000fffff;
static const uint64_t ACPI_SIG_RTP_DSR = 0x2052545020445352;
static const uint32_t ACPI_SIG_CIPA = 0x43495041;

ACPI* ACPI::getController() {
	if (controller) return controller;
	return controller = new ACPI();
}

ACPI::ACPI() {
	char *p = (char*)ACPI_FIND_START;
	char *end = (char*)ACPI_FIND_TOP;
	acpiCpuCount = 0;
	activeCpuCount = 1;
	
	if (!((CPU::getFeatures() >> 32) & CPUID_FEAT_APIC)) {
		acpiCpuCount = 1;
		return;
	}
	
	while (p < end) {
		Memory::salloc(p);
		Memory::salloc((uint64_t*)p+1);
		uint64_t signature = *(uint64_t *)p;
		
		if ((signature == ACPI_SIG_RTP_DSR) && ParseRsdp(p))
			break;
		
		p += 16;
	}
}
void ACPI::ParseDT(AcpiHeader *header) {
	Memory::salloc(header);
	
	if (header->signature == ACPI_SIG_CIPA)
		ParseApic((AcpiMadt *)header);
}
void ACPI::ParseRsdt(AcpiHeader *rsdt) {
	Memory::salloc(rsdt);
	uint32_t *p = (uint32_t *)(rsdt + 1);
	uint32_t *end = (uint32_t *)((char*)rsdt + rsdt->length);
	
	while (p < end) {
		Memory::salloc(p);
		Memory::salloc((_uint64*)p+1);
		_uint64 address = (_uint64)((uint32_t)(*p++) & 0xFFFFFFFF);
		ParseDT((AcpiHeader *)(uintptr_t)address);
	}
}
void ACPI::ParseXsdt(AcpiHeader *xsdt) {
	Memory::salloc(xsdt);
	uint64_t *p = (uint64_t *)(xsdt + 1);
	uint64_t *end = (uint64_t *)((char*)xsdt + xsdt->length);
	
	while (p < end) {
		Memory::salloc(p);
		Memory::salloc((_uint64*)p+1);
		uint64_t address = *p++;
		ParseDT((AcpiHeader *)(uintptr_t)address);
	}
}
void ACPI::ParseApic(AcpiMadt *a_madt) {
	Memory::salloc(a_madt);
	madt = a_madt;
	
	localApicAddr = (uint32_t *)((uintptr_t)madt->localApicAddr & 0xFFFFFFFF);
	Memory::salloc(localApicAddr);
	
	char *p = (char *)(madt + 1);
	char *end = ((char *)madt)+madt->header.length;
	
	while (p < end) {
		ApicHeader *header = (ApicHeader *)p;
		Memory::salloc(header);
		Memory::salloc(header+1);
		uint16_t type = header->type;
		uint16_t length = header->length;
		Memory::salloc(header);
		if (type == 0) {
			ApicLocalApic *s = (ApicLocalApic *)p;
			if (acpiCpuCount < 256) {
				acpiCpuIds[acpiCpuCount] = s->apicId;
				++acpiCpuCount;
			}
		} else if (type == 1) {
			ApicIoApic *s = (ApicIoApic *)p;
			ioApicAddr = (uint32_t *)(uintptr_t)((_uint64)s->ioApicAddress & 0xFFFFFFFF);
			Memory::salloc(ioApicAddr);
		} else if (type == 2) {
			// ApicInterruptOverride *s = (ApicInterruptOverride *)p;
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
	Memory::salloc((int *)(p + 16));
	Memory::salloc((int *)(p + 16)+1);
	if (revision == 0) {
		int rsdtAddr = *(int *)(p + 16);
		ParseRsdt((AcpiHeader *)(uintptr_t)((_uint64)rsdtAddr & 0xFFFFFFFF));
	} else if (revision == 2) {
		int rsdtAddr = *(int *)(p + 16);
		Memory::salloc((_uint64 *)(p + 24));
		Memory::salloc((_uint64 *)(p + 24)+1);
		_uint64 xsdtAddr = *(_uint64 *)(p + 24);
		
		if (xsdtAddr)
			ParseXsdt((AcpiHeader *)(uintptr_t)xsdtAddr);
		else
			ParseRsdt((AcpiHeader *)(uintptr_t)((_uint64)rsdtAddr & 0xFFFFFFFF));
	}
	
	return true;
}
uint32_t ACPI::getLapicID() {
	if (!localApicAddr) return 0;
	return LapicIn(LAPIC_APICID) >> 24;
}
uint32_t ACPI::getLapicIDOfCPU(uint32_t id) {
	if (!localApicAddr) return 0;
	return acpiCpuIds[id];
}
uint32_t ACPI::getCPUIDOfLapic(uint32_t id) {
	if (!localApicAddr) return 0;
	for (uint32_t i=0; i< 256; i++) {
		if (acpiCpuIds[i] == id) return i;
	}
	return 0;
}
uint32_t ACPI::LapicIn(uint32_t reg) {
	if (!localApicAddr) return 0;
	Memory::salloc((char*)localApicAddr + reg);
	return MmioRead32((char*)localApicAddr + reg);
}
void ACPI::LapicOut(uint32_t reg, uint32_t data) {
	if (!localApicAddr) return;
	Memory::salloc((char*)localApicAddr + reg);
	MmioWrite32((char*)localApicAddr + reg, data);
}
uint32_t ACPI::IOapicIn(uint32_t reg) {
	if (!ioApicAddr) return 0;
	Memory::salloc((char*)ioApicAddr + IOAPIC_REGSEL);
	Memory::salloc((char*)ioApicAddr + IOAPIC_REGWIN);
	MmioWrite32((char*)ioApicAddr + IOAPIC_REGSEL, reg);
	return MmioRead32((char*)ioApicAddr + IOAPIC_REGWIN);
}
void ACPI::IOapicOut(uint32_t reg, uint32_t data) {
	if (!ioApicAddr) return;
	Memory::salloc((char*)ioApicAddr + IOAPIC_REGSEL);
	Memory::salloc((char*)ioApicAddr + IOAPIC_REGWIN);
	MmioWrite32((char*)ioApicAddr + IOAPIC_REGSEL, reg);
	MmioWrite32((char*)ioApicAddr + IOAPIC_REGWIN, data);
}
union ioapic_redir_ints {
	ioapic_redir r;
	int i[2];
};
void ACPI::IOapicMap(uint32_t idx, ioapic_redir r) {
	ioapic_redir_ints a;
	a.r = r;
	IOapicOut(IOAPIC_REDTBL + idx*2 +0, a.i[0]);
	IOapicOut(IOAPIC_REDTBL + idx*2 +1, a.i[1]);
}
ioapic_redir ACPI::IOapicReadMap(uint32_t idx) {
	ioapic_redir_ints a;
	a.i[0] = IOapicIn(IOAPIC_REDTBL + idx*2 +0);
	a.i[1] = IOapicIn(IOAPIC_REDTBL + idx*2 +1);
	return a.r;
}
void* ACPI::getLapicAddr() {
	return localApicAddr;
}
uint32_t ACPI::getCPUCount() {
	if (!localApicAddr) return 1;
	return acpiCpuCount;
}
uint32_t ACPI::getActiveCPUCount() {
	if (!localApicAddr) return 1;
	return activeCpuCount;
}
void ACPI::activateCPU() {
	if (!localApicAddr) return;
	Interrupts::loadVector();
	LapicOut(LAPIC_DFR, 0xFFFFFFFF);
	LapicOut(LAPIC_LDR, (LapicIn(LAPIC_LDR)&0x00FFFFFF)|1);
	LapicOut(LAPIC_LVT_TMR, LAPIC_DISABLE);
	LapicOut(LAPIC_LVT_PERF, LAPIC_NMI);
	LapicOut(LAPIC_LVT_LINT0, LAPIC_DISABLE);
	LapicOut(LAPIC_LVT_LINT1, LAPIC_DISABLE);
	LapicOut(LAPIC_TASKPRIOR, 0);
	
	asm("mov $0x1B,%ecx \n rdmsr \n bts $11,%eax \n wrmsr");
	
	LapicOut(LAPIC_SPURIOUS, 0x27 | LAPIC_SW_ENABLE);
	
	_uint64 c = (ACPI::busfreq / 1000) >> 4;
	if (c < 0x10) c = 0x10;
	
	LapicOut(LAPIC_TMRINITCNT, c & 0xFFFFFFFF);
	LapicOut(LAPIC_LVT_TMR, 0x20 | TMR_PERIODIC);
	LapicOut(LAPIC_TMRDIV, 3);
	EOI();
	int ret_val;
	asm volatile("lock incq %1":"=a"(ret_val):"m"(activeCpuCount):"memory");
}
void ACPI::sendCPUInit(uint32_t id) {
	if (!localApicAddr) return;
	LapicOut(LAPIC_ICRH, id << 24);
	LapicOut(LAPIC_ICRL, 0x00004500);
	while (LapicIn(LAPIC_ICRL) & 0x00001000) {}
}
void ACPI::sendCPUStartup(uint32_t id, uint8_t vector) {
	if (!localApicAddr) return;
	LapicOut(LAPIC_ICRH, id << 24);
	LapicOut(LAPIC_ICRL, vector | 0x00004600);
	while (LapicIn(LAPIC_ICRL) & 0x00001000) {}
}
void ACPI::EOI() {
	if (!(ACPI::getController())->localApicAddr) return;
	(ACPI::getController())->LapicOut(LAPIC_EOI, 0);
}
void ACPI::initIOAPIC() {
	if (ioApicAddr == 0) return;
	ioApicMaxCount = (IOapicIn(IOAPIC_VER) >> 16) & 0xFF;
	ioapic_redir kbd;
	kbd.vector = 0x21;
	kbd.deliveryMode = 1;
	kbd.destinationMode = 0;
	kbd.deliveryStatus = 0;
	kbd.pinPolarity = 0;
	kbd.remoteIRR = 0;
	kbd.triggerMode = 0;
	kbd.mask = 0;
	kbd.destination = 0x00;
	IOapicMap(1, kbd);
}
bool ACPI::initAPIC() {
	if (!((CPU::getFeatures() >> 32) & CPUID_FEAT_APIC)) return false;
	if (localApicAddr == 0) return false;
	Interrupts::maskIRQ(0xFFFF);
	ACPI *acpi = (ACPI::getController());
	
	LapicOut(LAPIC_DFR, 0xFFFFFFFF);
	LapicOut(LAPIC_LDR, (LapicIn(LAPIC_LDR)&0x00FFFFFF)|1);
	LapicOut(LAPIC_LVT_TMR, LAPIC_DISABLE);
	LapicOut(LAPIC_LVT_PERF, LAPIC_NMI);
	LapicOut(LAPIC_LVT_LINT0, LAPIC_DISABLE);
	LapicOut(LAPIC_LVT_LINT1, LAPIC_DISABLE);
	LapicOut(LAPIC_TASKPRIOR, 0);
	
	asm("mov $0x1B,%ecx \n rdmsr \n bts $11,%eax \n wrmsr");
	
	LapicOut(LAPIC_SPURIOUS, 0x27 | LAPIC_SW_ENABLE);
	
	outportb(0x61, (inportb(0x61)&0xFD)|1);
	outportb(0x43, 0xB2);
	outportb(0x42, 0x9B);
	inportb(0x60);
	outportb(0x42, 0x2E);
	uint8_t t = inportb(0x61) & 0xFE;
	outportb(0x61, t);
	outportb(0x61, t|1);
	LapicOut(LAPIC_TMRINITCNT, -1);
	while ((inportb(0x61) & 0x20) == 0) {}
	LapicOut(LAPIC_LVT_TMR, LAPIC_DISABLE);
	acpi->busfreq = ((-1 - LapicIn(LAPIC_TMRCURRCNT)) << 4) * 100;
	uint64_t c = (ACPI::busfreq / 1000) >> 4;
	if (c < 0x10) c = 0x10;
	
	LapicOut(LAPIC_TMRINITCNT, c & 0xFFFFFFFF);
	LapicOut(LAPIC_LVT_TMR, 0x20 | TMR_PERIODIC);
	LapicOut(LAPIC_TMRDIV, 3);
	initIOAPIC();
	EOI();
	return true;
}
