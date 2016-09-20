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

DEP_SYSLINUX_VER = 6.03
DEP_SYSLINUX_URL = https://www.kernel.org/pub/linux/utils/boot/syslinux/6.xx/syslinux-6.03.zip

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
ISOROOT=$(ODIR)/iso

SRCDIR=src
MODDIR=modules

ASSEMBLY=$(wildcard $(SRCDIR)/*.s)
SOURCES=$(wildcard $(SRCDIR)/*.cpp)
MODULES=$(subst $(MODDIR)/,,$(wildcard $(MODDIR)/*))

MODOBJS=$(MODULES:%=$(OMODDIR)/%.o)

BIN=$(OIMGDIR)/pxkrnl

ifeq ($(UNAME_S),Darwin)
	OBJCOPY=gobjcopy
endif

.PHONY: all clean images kernel iso check

all: check kernel

define SRCOBJ
$(patsubst $(SRCDIR)/%.cpp, $(OOBJDIR)/%.o, $(patsubst $(SRCDIR)/%.s, $(OOBJDIR)/%.o, $(1)))
endef

OBJECTS=$(call SRCOBJ,$(SOURCES) $(ASSEMBLY))

define DEPSRC
DEPS := $$(DEPS) $$(patsubst %.o,%.d,$$(call SRCOBJ,$(1)))
endef

define SCANMOD
MOD_$(1)_SRCS := $$(wildcard $(MODDIR)/$(1)/*.cpp)
MODSRCS := $$(MODSRCS) $$(MOD_$(1)_SRCS)
$(OOBJDIR)/mod_$(1).o: $$(MOD_$(1)_SRCS)
	@ mkdir -p $$(dir $$@)
	@ echo MODCC $(1)
	@ $(CC) $(CFLAGS) $$^ -o $$@
endef

$(foreach mod, $(MODULES), $(eval $(call SCANMOD,$(mod))))
$(foreach src, $(SOURCES) $(ASSEMBLY), $(eval $(call DEPSRC,$(src))))

sinclude $(DEPS)

$(BIN).elf: $(OBJECTS) $(OOBJDIR)/modules-linked.o
	@ mkdir -p $(dir $@)
	@ echo LD $(subst $(OIMGDIR)/,,$@)
	@ $(LD) -T ld.script -belf64-x86-64 -o $@ -s --nostdlib $^

$(BIN): $(BIN).elf
	@ mkdir -p $(dir $@)
	@ echo OC $(subst $(OIMGDIR)/,,$@)
	@ $(OBJCOPY) -Opei-x86-64 --subsystem efi-app --file-alignment 1 --section-alignment 1 $^ $@

$(OOBJDIR)/%.d: $(SRCDIR)/%.cpp
	@ mkdir -p $(dir $@)
	@ $(CC) $(CFLAGS) -MM -MT $(call SRCOBJ,$^) $^ -o $@

$(OOBJDIR)/%.d: $(SRCDIR)/%.s
	@ mkdir -p $(dir $@)
	@ $(CC) -c -MM -MT $(call SRCOBJ,$^) $^ -o $@

$(OOBJDIR)/%.o: $(SRCDIR)/%.cpp
	@ mkdir -p $(dir $@)
	@ echo CC $<
	@ $(CC) $(CFLAGS) $< -o $@

$(OOBJDIR)/%.o: $(SRCDIR)/%.s
	@ mkdir -p $(dir $@)
	@ echo AS $<
	@ $(CC) -c -s $< -o $@

$(OMODDIR)/%.o: $(OOBJDIR)/mod_%.o
	@ mkdir -p $(dir $@)
	@ echo MODLD $(@:$(OMODDIR)/%.o=%)
	@ $(LD) -T ld-mod.script -r -belf64-x86-64 -o $@ -s --nostdlib $^

$(OOBJDIR)/modules-linked.o: $(MODOBJS)
	@ mkdir -p $(dir $@)
	@ cat $^ > $(@:.o=.b)
	@ $(OBJCOPY) -Oelf64-x86-64 -Bi386 -Ibinary --rename-section .data=.modules $(@:.o=.b) $@

clean:
	@ echo RM .output bin
	@ rm -rf .output bin

images: bin/phoenixos bin/phoenixos.iso

check: $(SOURCES) $(MODSRCS)
	@ cpplint --quiet $^ || echo "CPPLINT not found"

bin/phoenixos: $(BIN)
	@ mkdir -p $(dir $@)
	@ echo CP $@
	@ cp $^ $@

deps/syslinux-$(DEP_SYSLINUX_VER).zip:
	@ mkdir -p $(dir $@)
	@ echo DL $@
	@ wget -q '$(DEP_SYSLINUX_URL)' -O $@

bin/phoenixos.iso: $(BIN) deps/syslinux-$(DEP_SYSLINUX_VER).zip
	@ mkdir -p $(dir $@)
	@ mkdir -p $(ISOROOT)
	@ unzip -q -u -j deps/syslinux-$(DEP_SYSLINUX_VER).zip -d $(ISOROOT) bios/core/isolinux.bin bios/com32/elflink/ldlinux/ldlinux.c32 bios/com32/lib/libcom32.c32 bios/com32/mboot/mboot.c32
	@ cp $(BIN) $(ISOROOT)/phoenixos
	@ echo 'default /mboot.c32 /phoenixos' > $(ISOROOT)/isolinux.cfg
	@ echo ISO $@
	@ $(MKISOFS) -quiet -r -J -V 'PhoeniX OS' -o $@ -b isolinux.bin -c boot.cat -no-emul-boot -boot-load-size 4 -boot-info-table $(ISOROOT)/ || echo $(MKISOFS) not installed
	@ rm -rf $(ISOROOT)

kernel: bin/phoenixos
iso: bin/phoenixos.iso

launch:
	$(QEMU) -kernel $(BIN) -smp cores=2,threads=2 -cpu Nehalem
launch-efi: OVMF.fd
	$(QEMU) -kernel $(BIN) -smp cores=2,threads=2 -cpu Nehalem -bios OVMF.fd
