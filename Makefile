#    PhoeniX OS Makefile
#    Copyright (C) 2013  PhoeniX
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

PREFIX=x86_64-linux-gnu-
MKISOFS=genisoimage
ifeq ($(OS),Windows_NT)
    PREFIX=x86_64-w64-mingw32-
else
    UNAME_S=$(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        PREFIX=x86_64-elf-
		MKISOFS=mkisofs
    endif
endif

CC=$(PREFIX)gcc
LD=$(PREFIX)ld
AS=$(PREFIX)as
OBJCOPY=$(PREFIX)objcopy

CFLAGS := -c -s -m64
CFLAGS += -nostdlib -std=c++11
CFLAGS += -O2 -Wno-multichar -Wall
CFLAGS += -ffreestanding -fno-exceptions -fno-rtti
CFLAGS += -Iinclude

QEMU=qemu-system-x86_64

ODIR=.output
OOBJDIR=$(ODIR)/obj
OMODDIR=$(ODIR)/mod
OBINDIR=$(ODIR)/bin
OIMGDIR=$(ODIR)/img

SRCDIR=src
MODDIR=modules

ASSEMBLY=$(shell ls $(SRCDIR)/*.s)
SOURCES=$(shell ls $(SRCDIR)/*.cpp)
MODULES=$(shell ls $(MODDIR))

OBJECTS=$(ASSEMBLY:$(SRCDIR)/%.s=$(OOBJDIR)/%.o) $(SOURCES:$(SRCDIR)/%.cpp=$(OOBJDIR)/%.o)
MODOBJS=$(MODULES:%=$(OMODDIR)/%.o)

BIN=$(OIMGDIR)/pxkrnl

ifeq ($(UNAME_S),Darwin)
	OBJCOPY=gobjcopy
endif

.PHONY: all clean images kernel iso check

all: check kernel

$(BIN).elf: ${OBJECTS} $(OOBJDIR)/modules-linked.o
	@ mkdir -p $(dir $@)
	@ echo LD $@
	@ $(LD) -T ld.script -belf64-x86-64 -o $@ -s --nostdlib $?

$(BIN): $(BIN).elf
	@ mkdir -p $(dir $@)
	@ echo OC $@
	@ $(OBJCOPY) -Opei-x86-64 --subsystem efi-app --file-alignment 1 --section-alignment 1 $? $@

$(OOBJDIR)/%.o: $(SRCDIR)/%.cpp
	@ mkdir -p $(dir $@)
	@ echo CC $?
	@ $(CC) $(CFLAGS) $? -o $@

$(OOBJDIR)/%.o: $(SRCDIR)/%.s
	@ mkdir -p $(dir $@)
	@ echo AS $?
	@ $(CC) -c -s $? -o $@

define SCANMOD
MOD_$(1)_SRCS := $(foreach f, $(shell ls $(MODDIR)/$(1)), $(MODDIR)/$(1)/$(f))
MODSRCS := $$(MODSRCS) $$(MOD_$(1)_SRCS)
$(OOBJDIR)/mod_$(1).o: $$(MOD_$(1)_SRCS)
	@ mkdir -p $$(dir $$@)
	@ echo MODCC $(1)
	@ $(CC) $(CFLAGS) $$? -o $$@
endef

$(foreach mod, $(MODULES), $(eval $(call SCANMOD,$(mod))))

$(OMODDIR)/%.o: $(OOBJDIR)/mod_%.o
	@ mkdir -p $(dir $@)
	@ echo MODLD $(@:$(OMODDIR)/%.o=%)
	@ $(LD) -T ld-mod.script -r -belf64-x86-64 -o $@ -s --nostdlib $?

$(OOBJDIR)/modules-linked.o: $(MODOBJS)
	@ mkdir -p $(dir $@)
	@ cat $? > $(@:.o=.b)
	@ $(OBJCOPY) -Oelf64-x86-64 -Bi386 -Ibinary --rename-section .data=.modules $(@:.o=.b) $@

clean:
	@ echo RM .output bin
	@ rm -rf .output bin

images: bin/phoenixos bin/phoenixos.iso

check: $(SOURCES) $(MODSRCS)
	@ echo CPPLINT $?
	@ cpplint --quiet $? || echo "CPPLINT not found"

bin/phoenixos: $(BIN)
	@ mkdir -p $(dir $@)
	@ cp $^ $@

ISOROOT := $(ODIR)/iso

bin/phoenixos.iso: $(BIN) deps/syslinux.zip
	@ mkdir -p $(dir $@)
	@ mkdir -p $(ISOROOT)
	@ unzip -q -u -j deps/syslinux.zip -d $(ISOROOT) bios/core/isolinux.bin bios/com32/elflink/ldlinux/ldlinux.c32 bios/com32/lib/libcom32.c32 bios/com32/mboot/mboot.c32
	@ cp $(BIN) $(ISOROOT)/phoenixos
	@ echo 'default /mboot.c32 /phoenixos' > $(ISOROOT)/isolinux.cfg
	@ $(MKISOFS) -quiet -r -J -V 'PhoeniX OS' -o $@ -b isolinux.bin -c boot.cat -no-emul-boot -boot-load-size 4 -boot-info-table $(ISOROOT)/ || echo $(MKISOFS) not installed
	@ rm -rf $(ISOROOT)

kernel: bin/phoenixos
iso: bin/phoenixos.iso

launch:
	$(QEMU) -kernel $(BIN) -smp cores=2,threads=2 -cpu Nehalem
launch-efi: OVMF.fd
	$(QEMU) -kernel $(BIN) -smp cores=2,threads=2 -cpu Nehalem -bios OVMF.fd
