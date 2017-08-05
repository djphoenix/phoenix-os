PREFIX=x86_64-linux-gnu-
NPROC=1
CLANG_BIN?=$(dir $(shell which clang))
CLANG_BIN?=/usr/bin

ifeq ($(OS),Windows_NT)
  PREFIX=x86_64-w64-mingw32-
else
  UNAME_S=$(shell uname -s)
  ifeq ($(UNAME_S),Darwin)
    PREFIX=x86_64-elf-
    CLANG_BIN=/usr/local/opt/llvm/bin
	  NPROC=$(shell sysctl -n hw.ncpu)
  else
	  NPROC=$(shell grep -c ^processor /proc/cpuinfo)
  endif
endif

ifneq ($(PARALLEL),0)
MAKEFLAGS += "-j $(NPROC)"
endif

ifeq ($(VERBOSE),1)
  Q=
  QECHO=@ true
else
  Q=@
  QECHO=@ echo
endif

ifeq ($(wildcard $(CLANG_BIN)/clang),)
$(error CLANG not found in $(CLANG_BIN), make sure that it is installed and set CLANG_BIN environment variable)
endif

CC=$(CLANG_BIN)/clang --target=x86_64-none-elf
LD=$(CLANG_BIN)/ld.lld
AR=$(CLANG_BIN)/llvm-ar
OBJCOPY=$(PREFIX)objcopy
STRIP=$(PREFIX)strip

CLANG_VER=$(shell $(CLANG_BIN)/clang -v 2>&1 | head -n1 | perl -p -e 's/.*clang version (\d+)(\.\d+)*.*/$$1/g')
CLANG_5_GT=$(shell [ $(CLANG_VER) -gt 4 ] && echo true)

ifneq ($(CLANG_5_GT),true)
$(error CLANG v$(CLANG_VER) is not supported, use v5 or later)
endif

ifeq ($(UNAME_S),Darwin)
  OBJCOPY=gobjcopy
endif

QEMU=qemu-system-x86_64

ifneq ($(shell which cpplint),)
	LINT=$(shell which cpplint) --quiet
else
	LINT=@ sh -c "echo No lint found"; true
endif

ifneq ($(shell which xorriso),)
	MKISO := $(shell which xorriso) -as mkisofs
else
	ifneq ($(shell which mkisofs),)
		MKISO := $(shell which mkisofs)
	else
		ifneq ($(shell which genisoimage),)
			MKISO := $(shell which genisoimage)
		else
			MKISO=@ sh -c "echo Neither xorriso nor mkisofs nor genisoimage was found"; false
		endif
	endif
endif

ifneq ($(shell which mformat),)
	MKFAT := $(shell which mformat)
else
	MKFAT := @ sh -c "echo mformat not found"; false
endif

ifneq ($(shell which mcopy),)
	MCOPY := $(shell which mcopy)
else
  MCOPY := @ sh -c "echo mcopy not found"; false
endif

ifneq ($(shell which mmd),)
	MMD := $(shell which mmd)
else
  MMD := @ sh -c "echo mmd not found"; false
endif
