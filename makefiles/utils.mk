EFIBIOS=$(ODIR)/bios/OVMF.fd

.PHONY: clean check launch launch-efi

clean:
	@ echo RM .output bin
	@ rm -rf .output bin

check: $(SOURCES) $(MODSRCS)
	@ $(LINT) $^

all: check

$(EFIBIOS): deps/ovmf-$(DEP_OVMF_VER).zip
	@ mkdir -p $(dir $@)
	@ unzip -p deps/ovmf-r15214.zip OVMF.fd > $@

launch: $(BIN)
	$(QEMU) -kernel $< -smp cores=2,threads=2 -cpu Nehalem

launch-efi: $(BIN) $(EFIBIOS)
	$(QEMU) -kernel $< -smp cores=2,threads=2 -cpu Nehalem -bios $(EFIBIOS)
