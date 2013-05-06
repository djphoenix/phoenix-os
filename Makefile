CC=x86_64-w64-mingw32-gcc
LD=x86_64-w64-mingw32-ld
OBJCOPY=x86_64-w64-mingw32-objcopy
CFLAGS=-c -nostdlib -s -m64 -O2
BIN=bin/pxkrnl

all: $(BIN) clean
$(BIN): obj/x32to64.o obj/boot.o obj/memory.o obj/pxlib.o obj/interrupts.o obj/multiboot_info.o
	$(LD) -belf64-x86-64 -o $(BIN) -s --nostdlib $?
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
obj/x32to64.o : src/x32to64.s obj
	fasm src/x32to64.s obj/x32to64.o
clean:
	rm -rf obj
obj:
	mkdir -p obj