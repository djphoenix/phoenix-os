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
STRIP=$(PREFIX)strip

ifeq ($(UNAME_S),Darwin)
  OBJCOPY=gobjcopy
endif

QEMU=qemu-system-x86_64

ifneq ($(shell which cpplint),)
	LINT=$(shell which cpplint) --quiet
else
	LINT=sh -c "echo No lint found"
endif
