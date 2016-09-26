INCLUDES := include

CFLAGS := -c -g -m64
CFLAGS += -nostdlib -std=c++11
CFLAGS += -O2 -Wno-multichar -Wall
CFLAGS += -ffreestanding -fno-exceptions -fno-rtti
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += $(patsubst %,-I%,$(INCLUDES))

ODIR=.output
OOBJDIR=$(ODIR)/obj
OMODDIR=$(ODIR)/mod
OBINDIR=$(ODIR)/bin
OIMGDIR=$(ODIR)/img
ISOROOT=$(ODIR)/iso

SRCDIR=src
MODDIR=modules

ASSEMBLY=$(wildcard $(SRCDIR)/*.s)
SOURCES=$(wildcard $(SRCDIR)/*.cpp)
MODULES=$(subst $(MODDIR)/,,$(wildcard $(MODDIR)/*))
HEADERS=$(foreach inc,$(INCLUDES),$(wildcard $(inc)/*.h) $(wildcard $(inc)/*.hpp))

MODOBJS=$(MODULES:%=$(OMODDIR)/%.o)

BIN=$(OIMGDIR)/pxkrnl

OBJECTS=$(call SRCOBJ,$(SOURCES) $(ASSEMBLY))
