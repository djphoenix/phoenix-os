CFLAGS := -g -m64 -mno-sse
CFLAGS += -nostdlib -std=c++11
CFLAGS += -O2 -Os -Wall -Wextra -Werror
CFLAGS += -Wsuggest-attribute=format -Wsuggest-attribute=pure
CFLAGS += -Wsuggest-attribute=const -Wsuggest-attribute=noreturn
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
