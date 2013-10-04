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

PREFIX=
ifeq ($(OS),Windows_NT)
    PREFIX=x86_64-w64-mingw32-
endif

CC=$(PREFIX)gcc
LD=$(PREFIX)ld
OBJCOPY=$(PREFIX)objcopy
CFLAGS=-c -nostdlib -s -m64 -O2
BIN=bin/pxkrnl

all: $(BIN) clean
$(BIN): obj/x32to64.o obj/boot.o obj/memory.o obj/pxlib.o obj/interrupts.o obj/multiboot_info.o obj/modules.o obj/smp_init.o obj/smp.o obj/modules-linked.o
	$(LD) -T ld.script -belf64-x86-64 -o $(BIN) -s --nostdlib $?
	$(OBJCOPY) $(BIN) -Felf32-i386
obj/boot.o : src/boot.cpp obj
	$(CC) $(CFLAGS) src/boot.cpp -o $@
obj/pxlib.o : src/pxlib.cpp obj
	$(CC) $(CFLAGS) src/pxlib.cpp -o $@
obj/memory.o : src/memory.cpp obj
	$(CC) $(CFLAGS) src/memory.cpp -o $@
obj/smp.o : src/smp.cpp obj
	$(CC) $(CFLAGS) src/smp.cpp -o $@
obj/interrupts.o : src/interrupts.cpp obj
	$(CC) $(CFLAGS) src/interrupts.cpp -o $@
obj/multiboot_info.o : src/multiboot_info.cpp obj
	$(CC) $(CFLAGS) src/multiboot_info.cpp -o $@
obj/modules.o : src/modules.cpp obj
	$(CC) $(CFLAGS) -Wno-multichar src/modules.cpp -o $@
obj/x32to64.o : src/x32to64.s obj
	fasm src/x32to64.s obj/x32to64.o
obj/smp_init.o : src/smp.s obj
	fasm src/smp.s obj/smp_init.b
	$(OBJCOPY) -Oelf64-x86-64 -Bi386 -Ibinary --rename-section .data=.smp_init obj/smp_init.b obj/smp_init.o
obj/modules-linked.o : mod/hello.o
	$(OBJCOPY) -Oelf64-x86-64 -Bi386 -Ibinary --rename-section .data=.modules $? $@
mod/hello.o : modules/hello/hello.cpp
	$(CC) $(CFLAGS) $? -o $@
	$(OBJCOPY) -Oelf64-x86-64 $@
clean:
	rm -rf obj mod
obj:
	mkdir -p obj mod
launch:
	qemu -kernel $(BIN) -smp 8
