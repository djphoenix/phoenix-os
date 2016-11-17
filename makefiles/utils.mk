EFIBIOS=$(ODIR)/bios/OVMF.fd
EFIROOT=$(ODIR)/efi
EFIKERN=$(EFIROOT)/efi/boot/bootx64.efi

.PHONY: clean check launch launch-efi

clean:
	$(QECHO) RM $(ODIR) bin
	$(Q) rm -rf $(ODIR) bin

CHECKSRCS := $(filter %.cpp,$(SOURCES))
CHECKHDRS := $(foreach i,$(INCLUDES),$(wildcard $(i)/*.hpp))

$(ODIR)/check-report.txt: $(CHECKSRCS) $(CHECKHDRS)
	$(Q) $(LINT) $^ > $@ || rm $@

check: $(ODIR)/check-report.txt
	@ [ -f $< ] && cat $<

all: check

$(EFIBIOS): deps/$(DEP_OVMF_FILE)
	@ mkdir -p $(dir $@)
	$(Q) unzip -p deps/$(DEP_OVMF_FILE) OVMF.fd > $@

$(EFIKERN): $(BIN)
	@ mkdir -p $(dir $@)
	$(Q) cp $< $@

launch: $(BIN)
	$(QEMU) -kernel $< -smp cores=2,threads=2 -cpu Broadwell -serial stdio

launch-efi: $(EFIKERN) $(EFIBIOS)
	$(QEMU) -drive file=fat:$(EFIROOT) -smp cores=2,threads=2 -cpu Broadwell -bios $(EFIBIOS) -serial stdio
