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
#include "process.hpp"

struct GDT_ENT {
	uint64_t seg_lim_low:16;
	uint64_t base_low:24;
	uint8_t type:4;
	bool system:1;
	uint8_t dpl:2;
	bool present:1;
	uint64_t seg_lim_high:4;
	bool avl:1;
	bool islong:1;
	bool db:1;
	bool granularity:1;
	uint64_t base_high:8;
} __attribute__((packed));

struct GDT_SYS_ENT {
	GDT_ENT ent;
	uint64_t base_high:32;
	uint32_t rsvd;
} __attribute__((packed));

struct {
	uint16_t size;
	uintptr_t base;
} __attribute__((packed)) gdtrec;

struct GDT_ENT_def {
	uint64_t base, limit;
	uint8_t type, dpl;
	bool system, present, avl;
	bool islong, db, granularity;
};

GDT_ENT inline gdt_encode(GDT_ENT_def def) {
	return {
		.seg_lim_low = def.limit & 0xFFFF,
		.base_low = def.base & 0xFFFFFF,
		.type = def.type, .system = def.system,
		.dpl = def.dpl, .present = def.present,
		.seg_lim_high = (def.limit >> 16) & 0xF,
		.avl = def.avl, .islong = def.islong,
		.db = def.db, .granularity = def.granularity,
		.base_high = (def.base >> 24) & 0xFF
	};
}

GDT_ENT_def inline gdt_decode(GDT_ENT ent) {
	return {
		.base = ((uint64_t)ent.base_high << 24) | ent.base_low,
		.limit = ((uint64_t)ent.seg_lim_high << 16) | ent.seg_lim_low,
		.type = ent.type, .dpl = ent.dpl,
		.system = ent.system, .present = ent.present,
		.avl = ent.avl, .islong = ent.islong,
		.db = ent.db, .granularity = ent.granularity
	};
}

GDT_SYS_ENT inline gdt_sys_encode(GDT_ENT_def def) {
	return {
		.ent = {
			.seg_lim_low = def.limit & 0xFFFF,
			.base_low = def.base & 0xFFFFFF,
			.type = def.type, .system = def.system,
			.dpl = def.dpl, .present = def.present,
			.seg_lim_high = (def.limit >> 16) & 0xF,
			.avl = def.avl, .islong = def.islong,
			.db = def.db, .granularity = def.granularity,
			.base_high = (def.base >> 24) & 0xFF
		},
		.base_high = def.base >> 32,
		.rsvd = 0
	};
}

GDT_ENT_def inline gdt_sys_decode(GDT_SYS_ENT ent_sys) {
	GDT_ENT ent = ent_sys.ent;
	return {
		.base = (ent_sys.base_high << 32) | (ent.base_high << 24) | ent.base_low,
		.limit = ((uint64_t)ent.seg_lim_high << 16) | ent.seg_lim_low,
		.type = ent.type, .dpl = ent.dpl,
		.system = ent.system, .present = ent.present,
		.avl = ent.avl, .islong = ent.islong,
		.db = ent.db, .granularity = ent.granularity
	};
}

struct TSS64_ENT {
	uint32_t reserved1;
	uint64_t rsp[3];
	uint64_t reserved2;
	uint64_t ist[7];
	uint64_t reserved3;
	uint16_t reserved4;
	uint16_t iomap_base;
} __attribute__((packed));

static const GDT_ENT GDT_ENT_zero = {};

GDT_ENT *gdt = 0;
TSS64_ENT *tss = 0;

void SMP::init_gdt(uint32_t ncpu) {
	gdt = (GDT_ENT*)Memory::alloc(
								  5 * sizeof(GDT_ENT) +
								  ncpu * sizeof(GDT_SYS_ENT));
	tss = (TSS64_ENT*)Memory::alloc(ncpu * sizeof(TSS64_ENT));
	gdtrec.base = (uintptr_t)gdt;
	gdtrec.size = 5 * sizeof(GDT_ENT) + ncpu * sizeof(GDT_SYS_ENT);
	gdt[0] = GDT_ENT_zero;
	gdt[1] = gdt_encode({
		.base = 0, .limit = 0,
		.type = 0xA, .dpl = 0,
		.system = 1, .present = 1,
		.avl = 0, .islong = 1,
		.db = 0, .granularity = 0
	});
	gdt[2] = gdt_encode({
		.base = 0, .limit = 0,
		.type = 0x2, .dpl = 0,
		.system = 1, .present = 1,
		.avl = 0, .islong = 1,
		.db = 0, .granularity = 0
	});
	gdt[3] = gdt_encode({
		.base = 0, .limit = 0xFFFFFFFFFFFFFFFF,
		.type = 0xA, .dpl = 3,
		.system = 1, .present = 1,
		.avl = 0, .islong = 1,
		.db = 0, .granularity = 0
	});
	gdt[4] = gdt_encode({
		.base = 0, .limit = 0xFFFFFFFFFFFFFFFF,
		.type = 0x2, .dpl = 3,
		.system = 1, .present = 1,
		.avl = 0, .islong = 1,
		.db = 0, .granularity = 0
	});
	GDT_SYS_ENT *gdtsys = (GDT_SYS_ENT*)&gdt[5];
	for (uint32_t idx = 0; idx < ncpu; idx++) {
		void *stack = Memory::palloc(1);
		uintptr_t stack_ptr = (uintptr_t)stack + 0x1000;
		tss[idx] = {
			.reserved1 = 0,
			.rsp = {
				0, 0, 0
			},
			.reserved2 = 0,
			.ist = {
				stack_ptr, 0, 0, 0, 0, 0, 0
			},
			.reserved3 = 0,
			.reserved4 = 0,
			.iomap_base = 0
		};
		gdtsys[idx] = gdt_sys_encode({
			.base = (uintptr_t)&tss[idx],
			.limit = sizeof(TSS64_ENT),
			.type = 0x9, .dpl = 0,
			.system = 0, .present = 1,
			.avl = 0, .islong = 1,
			.db = 0, .granularity = 0
		});
	}
}

void SMP::setup_gdt() {
	ACPI* acpi = ACPI::getController();
	if (gdt == 0) init_gdt(acpi->getCPUCount());
	uint32_t cpuid = acpi->getCPUID();
	uint16_t tr = 5*sizeof(GDT_ENT) + cpuid*sizeof(GDT_SYS_ENT);
	asm volatile("pushfq; cli");
	asm volatile("lgdtq %0"::"m"(gdtrec));
	asm volatile("ltr %w0"::"a"(tr));
	asm volatile("popfq");
}

Mutex cpuinit = Mutex();

void SMP::startup() {
	setup_gdt();
	ACPI::getController()->activateCPU();
	cpuinit.lock(); cpuinit.release();
	process_loop();
}

extern "C" {
	extern void *_smp_init, *_smp_end;
}

static inline void __msleep(uint64_t milliseconds) {
	milliseconds *= 1000;
	asm volatile("pushfq; cli");

	outportb(0x61, (inportb(0x61)&0xFD)|1);
	outportb(0x43, 0xB2);
	outportb(0x42, 0xA9);
	outportb(0x42, 0x04);
	while (milliseconds--) {
		uint8_t t = inportb(0x61) & 0xFE;
		outportb(0x61, t);
		outportb(0x61, t|1);
		while ((inportb(0x61) & 0x20) == 0) {}
	}

	asm volatile("popfq");
}

void SMP::init() {
	setup_gdt();
	Interrupts::setIST(1);
	ACPI* acpi = ACPI::getController();
	uint32_t localId = acpi->getLapicID();
	uint32_t cpuCount = acpi->getCPUCount();
	if (cpuCount == 1) return;
	
	uintptr_t *smp_init_code = (uintptr_t*)Memory::palloc(1);
	uintptr_t *stacks = (uintptr_t*)Memory::alloc(sizeof(uintptr_t)*cpuCount);
	uintptr_t *cpuids = (uintptr_t*)Memory::alloc(sizeof(uintptr_t)*cpuCount);
	uint32_t nullcpus = 0;
	for(uint32_t i = 0; i < cpuCount; i++) {
		cpuids[i] = acpi->getLapicIDOfCPU(i);
		if(cpuids[i] != localId)
			stacks[i] = ((uintptr_t)Memory::palloc(1))+0x1000;
		else
			nullcpus++;
	}
	if (nullcpus > 0) nullcpus--;

	char smp_init_vector = (((uintptr_t)smp_init_code) >> 12) & 0xFF;
	uintptr_t *ptr = (uintptr_t*)&_smp_init;
	while (ptr < (uintptr_t*)&_smp_end)
		*(smp_init_code++) = *(ptr++);
	
	*(smp_init_code++) = (uintptr_t)acpi->getLapicAddr();
	*(smp_init_code++) = (uintptr_t)cpuids;
	*(smp_init_code++) = (uintptr_t)stacks;
	*(smp_init_code++) = (uintptr_t)(&SMP::startup);
	
	cpuinit.lock();
	
	for(uint32_t i = 0; i < cpuCount; i++)
		if(cpuids[i] != localId)
			acpi->sendCPUInit(cpuids[i]);
	__msleep(10);
	for(uint32_t i = 0; i < cpuCount; i++)
		if(cpuids[i] != localId)
			acpi->sendCPUStartup(cpuids[i], smp_init_vector);
	__msleep(10);
	for(uint32_t i = 0; i < cpuCount; i++)
		if(cpuids[i] != localId)
			acpi->sendCPUStartup(cpuids[i], smp_init_vector);
	while (cpuCount - nullcpus != acpi->getActiveCPUCount())
		__msleep(10);
	
	cpuinit.release();
	
	Memory::free(cpuids);
	Memory::free(stacks);
	Memory::pfree(smp_init_code);
}
