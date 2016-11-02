bin/phoenixos: $(BIN)
	@ mkdir -p $(dir $@)
	$(QECHO) CP $@
	$(Q) cp $^ $@

SYSLINUX_DEP_FILELIST := \
	bios/core/isolinux.bin \
	bios/com32/elflink/ldlinux/ldlinux.c32 \
	bios/com32/lib/libcom32.c32 \
	bios/com32/mboot/mboot.c32

SYSLINUX_FILELIST := $(notdir $(SYSLINUX_DEP_FILELIST))

ISOFILES := $(foreach f, $(SYSLINUX_FILELIST) phoenixos isolinux.cfg, $(ISOROOT)/$(f))

define SYSLINUX_EXTRACT
$(ISOROOT)/$(1): deps/$(DEP_SYSLINUX_FILE)
	@ mkdir -p $$(dir $$@)
	$(Q) unzip -q -u -j deps/$(DEP_SYSLINUX_FILE) -d $(ISOROOT) $(SYSLINUX_DEP_FILELIST)
	@ touch $(foreach f, $(SYSLINUX_FILELIST), $(ISOROOT)/$(f))
endef

$(foreach f,$(SYSLINUX_FILELIST),$(eval $(call SYSLINUX_EXTRACT,$(f))))

$(ISOROOT)/phoenixos: $(BIN)
	@ mkdir -p $(dir $@)
	$(Q) cp $< $@

$(ISOROOT)/isolinux.cfg:
	@ mkdir -p $(dir $@)
	$(Q) echo 'default /mboot.c32 /phoenixos' > $(ISOROOT)/isolinux.cfg

ISOOPTS := -quiet -U -A $(ISONAME) -V $(ISONAME) -volset $(ISONAME) -J -joliet-long -r
ISOOPTS += -b isolinux.bin -c boot.cat -no-emul-boot -boot-load-size 4 -boot-info-table

bin/phoenixos.iso: $(ISOFILES)
	@ mkdir -p $(dir $@)
	$(QECHO) ISO $@
	@ rm -f $@
	$(Q) $(MKISO) $(ISOOPTS) -o $@ $(ISOROOT)

.PHONY: images kernel iso

kernel: bin/phoenixos
iso: bin/phoenixos.iso

images: kernel iso
