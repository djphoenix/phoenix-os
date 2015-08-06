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
else
    UNAME_S=$(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        PREFIX=x86_64-elf-
    endif
endif

CC=$(PREFIX)gcc
LD=$(PREFIX)ld
OBJCOPY=$(PREFIX)objcopy
CFLAGS=-c -nostdlib -s -m64 -O2 -Wno-multichar -fno-exceptions -fno-rtti
BIN=bin/pxkrnl
ASSEMBLY=$(shell ls src/*.s)
SOURCES=$(shell ls src/*.cpp)
OBJECTS=$(ASSEMBLY:src/%.s=obj/%.o) $(SOURCES:src/%.cpp=obj/%.o)
MODULES=hello

ifeq ($(UNAME_S),Darwin)
    OBJCOPY=gobjcopy
endif

all: $(BIN) clean
$(BIN): ${OBJECTS} obj/modules-linked.o
	$(LD) -T ld.script -belf64-x86-64 -o $(BIN).elf -s --nostdlib $?
	$(OBJCOPY) -Opei-x86-64 $(BIN).elf $(BIN)

obj/%.o: src/%.cpp obj
	$(CC) $(CFLAGS) $< -o $@
obj/%.o: src/%.s obj
	fasm $< $@
obj/smp_init.o: src/smp_init.s obj
	fasm src/smp_init.s obj/smp_init.b
	$(OBJCOPY) -Oelf64-x86-64 -Bi386 -Ibinary --rename-section .data=.smp_init obj/smp_init.b obj/smp_init.o

obj/modules-linked.o: obj
	for mod in $(MODULES); do \
		$(CC) $(CFLAGS) modules/$$mod/$$mod.cpp -o obj/mod/$$mod.o ;\
		$(OBJCOPY) -Oelf64-x86-64 obj/mod/$$mod.o -R '*.eh_frame' -R .comment -R '.note.*' ;\
	done
	cat $(MODULES:%=obj/mod/%.o) > $(@:.o=.b)
	$(OBJCOPY) -Oelf64-x86-64 -Bi386 -Ibinary --rename-section .data=.modules $(@:.o=.b) $@

clean:
	rm -rf obj
obj:
	mkdir -p obj/mod
launch:
	qemu -kernel $(BIN) -smp 8
