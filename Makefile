#    PhoeniX OS Makefile
#    Copyright © 2017 Yury Popov a.k.a. PhoeniX

.PHONY: all

all: kernel

MAKEROOT := $(PWD)/makefiles

sinclude $(MAKEROOT)/root.mk
