PREFIX=x86_64-linux-gnu-
NPROC=1

ifeq ($(OS),Windows_NT)
  PREFIX=x86_64-w64-mingw32-
else
  UNAME_S=$(shell uname -s)
  ifeq ($(UNAME_S),Darwin)
    PREFIX=x86_64-elf-
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

CC=$(PREFIX)g++
LD=$(PREFIX)g++
AR=$(PREFIX)ar
OBJCOPY=$(PREFIX)objcopy
STRIP=$(PREFIX)strip

ifeq ($(UNAME_S),Darwin)
  OBJCOPY=gobjcopy
endif

QEMU=qemu-system-x86_64

ifneq ($(shell which cpplint),)
	LINT=$(shell which cpplint) --quiet
else
	LINT=@ sh -c "echo No lint found"
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
			MKISO=@ sh -c "echo Neither xorriso nor mkisofs nor genisoimage was found"
		endif
	endif
endif

