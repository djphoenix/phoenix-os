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

#ifndef ACPI_H
#define ACPI_H
#include "pxlib.hpp"
#include "memory.hpp"
#include "cpu.hpp"

enum LAPIC_FIELDS {
	LAPIC_APICID        = 0x20,
	LAPIC_APICVER       = 0x30,
	LAPIC_TASKPRIOR     = 0x80,
	LAPIC_EOI           = 0xB0,
	LAPIC_LDR           = 0xD0,
	LAPIC_DFR           = 0xE0,
	LAPIC_SPURIOUS      = 0xF0,
	LAPIC_ESR           = 0x280,
	LAPIC_ICRL          = 0x300,
	LAPIC_ICRH          = 0x310,
	LAPIC_LVT_TMR       = 0x320,
	LAPIC_LVT_PERF      = 0x340,
	LAPIC_LVT_LINT0     = 0x350,
	LAPIC_LVT_LINT1     = 0x360,
	LAPIC_LVT_ERR       = 0x370,
	LAPIC_TMRINITCNT	= 0x380,
	LAPIC_TMRCURRCNT	= 0x390,
	LAPIC_TMRDIV        = 0x3E0,
	LAPIC_LAST          = 0x38F,
};
enum LAPIC_VALUES {
	LAPIC_DISABLE       = 0x10000,
	LAPIC_SW_ENABLE     = 0x100,
	LAPIC_CPUFOCUS      = 0x200,
	LAPIC_NMI           = (4<<8),
	TMR_PERIODIC        = 0x20000,
	TMR_BASEDIV         = (1<<20),
};

enum IOAPIC_FIELDS {
	IOAPIC_REGSEL       = 0x0,
	IOAPIC_REGWIN       = 0x10,
};

enum IOAPIC_REGS {
	IOAPIC_ID           = 0x0,
	IOAPIC_VER          = 0x1,
	IOAPIC_REDTBL       = 0x10,
};

typedef unsigned int *uintptr_t;
typedef struct
{
	int signature;
	int length;
	char revision;
	char checksum;
	char oem[6];
	char oemTableId[8];
	int oemRevision;
	int creatorId;
	int creatorRevision;
} AcpiHeader;
typedef struct AcpiMadt
{
	AcpiHeader header;
	int localApicAddr;
	int flags;
} AcpiMadt;
typedef struct ApicHeader
{
	char type;
	char length;
} ApicHeader;
typedef struct ApicLocalApic
{
	ApicHeader header;
	unsigned char acpiProcessorId;
	unsigned char apicId;
	unsigned int flags;
} ApicLocalApic;
typedef struct ApicIoApic
{
	ApicHeader header;
	char ioApicId;
	char reserved;
	int ioApicAddress;
	int globalSystemInterruptBase;
} ApicIoApic;
typedef struct ApicInterruptOverride
{
	ApicHeader header;
	char bus;
	char source;
	int interrupt;
	short flags;
} ApicInterruptOverride;

typedef struct {
	char vector: 8;
	char deliveryMode: 3;
	bool destinationMode: 1;
	bool deliveryStatus: 1;
	bool pinPolarity: 1;
	bool remoteIRR: 1;
	bool triggerMode: 1;
	bool mask: 1;
	_uint64 reserved: 39;
	char destination: 8;
} ioapic_redir;

class ACPI {
private:
	AcpiMadt *madt;
	long* localApicAddr, *ioApicAddr;
	char ioApicMaxCount;
	int acpiCpuIds[256];
	unsigned char acpiCpuCount;
	static unsigned char activeCpuCount;
	static _uint64 busfreq;
	static ACPI *controller;
	static Mutex *cpuCountMutex;
	bool ParseRsdp(char* rsdp);
	void ParseRsdt(AcpiHeader* rsdt);
	void ParseXsdt(AcpiHeader* xsdt);
	void ParseDT(AcpiHeader* dt);
	void ParseApic(AcpiMadt *madt);
	void initIOAPIC();
public:
	ACPI();
	static ACPI* getController();
	int getLapicID();
	void* getLapicAddr();
	int getCPUCount();
	int getActiveCPUCount();
	int LapicIn(int reg);
	void LapicOut(int reg, int data);
	int IOapicIn(int reg);
	void IOapicOut(int reg, int data);
	void IOapicMap(int idx, ioapic_redir r);
	ioapic_redir IOapicReadMap(int idx);
	void activateCPU();
	int getLapicIDOfCPU(int cpuId);
	int getCPUIDOfLapic(int lapicId);
	void sendCPUInit(int id);
	void sendCPUStartup(int id, char vector);
	bool initAPIC();
	static void EOI();
};

#endif