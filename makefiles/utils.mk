EFIBIOS=$(ODIR)/bios/OVMF.fd
EFIROOT=$(ODIR)/efi
EFIKERN=$(EFIROOT)/efi/boot/bootx64.efi

.PHONY: clean check launch launch-efi

clean:
	@ echo RM .output bin
	@ rm -rf .output bin

check: $(SOURCES) $(MODSRCS) $(HEADERS)
	@ $(LINT) $^

all: check

$(EFIBIOS): deps/ovmf-$(DEP_OVMF_VER).zip
	@ mkdir -p $(dir $@)
	@ unzip -p deps/ovmf-r15214.zip OVMF.fd > $@

$(EFIKERN): $(BIN)
	@ mkdir -p $(dir $@)
	@ cp $< $@

launch: $(BIN)
	$(QEMU) -kernel $< -smp cores=2,threads=2 -cpu Nehalem

launch-efi: $(EFIKERN) $(EFIBIOS)
	$(QEMU) -drive file=fat:$(EFIROOT) -smp cores=2,threads=2 -cpu Nehalem -bios $(EFIBIOS)
