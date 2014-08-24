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

#ifndef SMP_H
#define SMP_H
#include "memory.hpp"
extern void smp_init();
typedef unsigned int *uintptr_t;
typedef struct AcpiHeader
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
#endif
