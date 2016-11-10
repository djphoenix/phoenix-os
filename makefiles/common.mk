CFLAGS := -g -m64 -mno-sse
CFLAGS += -nostdlib -std=c++11
CFLAGS += -O2 -Os -Wall -Wextra -Werror -Wno-multichar
CFLAGS += -ffreestanding -fno-exceptions -fno-rtti -fshort-wchar
CFLAGS += -ffunction-sections -fdata-sections -fpic -fpie

ODIR=.output
OOBJDIR=$(ODIR)/obj
OMODDIR=$(ODIR)/mod
OBINDIR=$(ODIR)/bin
OLIBDIR=$(ODIR)/lib
OIMGDIR=$(ODIR)/img
ISOROOT=$(ODIR)/iso

SRCDIR=src

BIN=$(OIMGDIR)/pxkrnl
