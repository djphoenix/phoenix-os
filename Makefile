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
CFLAGS=-c -nostdlib -std=c++11 -s -m64 -O2 -Wno-multichar -ffreestanding -fno-exceptions -fno-rtti -Wall
BIN=obj/pxkrnl
ASSEMBLY=$(shell ls src/*.s)
SOURCES=$(shell ls src/*.cpp)
OBJECTS=$(ASSEMBLY:src/%.s=obj/%.o) $(SOURCES:src/%.cpp=obj/%.o)
MODULES=hello
QEMU=qemu-system-x86_64

ifeq ($(UNAME_S),Darwin)
    OBJCOPY=gobjcopy
endif

all: clean check $(BIN)
$(BIN): ${OBJECTS} obj/modules-linked.o
	$(LD) -T ld.script -belf64-x86-64 -o $(BIN).elf -s --nostdlib $?
	$(OBJCOPY) -Opei-x86-64 --subsystem efi-app --file-alignment 1 --section-alignment 1 $(BIN).elf $(BIN)

obj/%.o: src/%.cpp obj
	$(CC) $(CFLAGS) $< -o $@
obj/%.o: src/%.s obj
	$(AS) -c -s $< -o $@

obj/modules-linked.o: obj
	for mod in $(MODULES); do \
		$(CC) $(CFLAGS) modules/$$mod/$$mod.cpp -o obj/mod/$$mod.mo ;\
		$(LD) -T ld-mod.script -r -belf64-x86-64 -o obj/mod/$$mod.o -s --nostdlib obj/mod/$$mod.mo ;\
	done
	cat $(MODULES:%=obj/mod/%.o) > $(@:.o=.b)
	$(OBJCOPY) -Oelf64-x86-64 -Bi386 -Ibinary --rename-section .data=.modules $(@:.o=.b) $@

clean:
	rm -rf obj $(BIN).elf $(BIN) phoenixos phoenixos.iso
obj:
	mkdir -p obj/mod
images: phoenixos phoenixos.iso
check:
	cpplint $(SOURCES) modules/*/*.cpp || echo "CPPLINT not found"

phoenixos: $(BIN)
	cp $< $@

phoenixos.iso: $(BIN) deps/syslinux.zip
	mkdir -p .isoroot
	unzip -u -j deps/syslinux.zip -d .isoroot bios/core/isolinux.bin bios/com32/elflink/ldlinux/ldlinux.c32 bios/com32/lib/libcom32.c32 bios/com32/mboot/mboot.c32
	cp $(BIN) .isoroot/phoenixos
	echo 'default /mboot.c32 /phoenixos' > .isoroot/isolinux.cfg
	$(MKISOFS) -r -J -V 'PhoeniX OS' -o phoenixos.iso -b isolinux.bin -c boot.cat -no-emul-boot -boot-load-size 4 -boot-info-table .isoroot/ || echo $(MKISOFS) not installed
	rm -rf .isoroot

launch:
	$(QEMU) -kernel $(BIN) -smp cores=2,threads=2 -cpu Nehalem
launch-efi: OVMF.fd
	$(QEMU) -kernel $(BIN) -smp cores=2,threads=2 -cpu Nehalem -bios OVMF.fd
