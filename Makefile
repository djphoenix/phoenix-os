CC=x86_64-w64-mingw32-gcc
LD=x86_64-w64-mingw32-ld
OBJCOPY=x86_64-w64-mingw32-objcopy
CFLAGS=-c -nostdlib -s -m64 -O2
BIN=bin/pxkrnl

all: $(BIN) clean
$(BIN): obj/x32to64.o obj/boot.o obj/memory.o obj/pxlib.o obj/interrupts.o obj/multiboot_info.o obj/modules.o obj/modules-linked.o
	$(LD) -T ld.script -belf64-x86-64 -o $(BIN) -s --nostdlib $?
	$(OBJCOPY) $(BIN) -Felf32-i386
obj/boot.o : src/boot.cpp obj
	$(CC) $(CFLAGS) src/boot.cpp -o $@
obj/pxlib.o : src/pxlib.cpp obj
	$(CC) $(CFLAGS) src/pxlib.cpp -o $@
obj/memory.o : src/memory.cpp obj
	$(CC) $(CFLAGS) src/memory.cpp -o $@
obj/interrupts.o : src/interrupts.cpp obj
	$(CC) $(CFLAGS) src/interrupts.cpp -o $@
obj/multiboot_info.o : src/multiboot_info.cpp obj
	$(CC) $(CFLAGS) src/multiboot_info.cpp -o $@
obj/modules.o : src/modules.cpp obj
	$(CC) $(CFLAGS) src/modules.cpp -o $@
obj/x32to64.o : src/x32to64.s obj
	fasm src/x32to64.s obj/x32to64.o
obj/modules-linked.o : mod/hello.o
	$(OBJCOPY) -Oelf64-x86-64 -Bi386 -Ibinary --rename-section .data=.modules $? $@
mod/hello.o : modules/hello/hello.cpp
	$(CC) $(CFLAGS) $? -o $@
	$(OBJCOPY) -Oelf64-x86-64 -Bi386 $@
clean:
	rm -rf obj mod
obj:
	mkdir -p obj mod
