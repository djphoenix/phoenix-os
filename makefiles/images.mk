bin/phoenixos: $(BIN)
	@ mkdir -p $(dir $@)
	@ echo CP $@
	@ cp $^ $@

bin/phoenixos.iso: $(BIN) deps/syslinux-$(DEP_SYSLINUX_VER).zip
	@ mkdir -p $(dir $@)
	@ mkdir -p $(ISOROOT)
	@ unzip -q -u -j deps/syslinux-$(DEP_SYSLINUX_VER).zip -d $(ISOROOT) bios/core/isolinux.bin bios/com32/elflink/ldlinux/ldlinux.c32 bios/com32/lib/libcom32.c32 bios/com32/mboot/mboot.c32
	@ cp $(BIN) $(ISOROOT)/phoenixos
	@ echo 'default /mboot.c32 /phoenixos' > $(ISOROOT)/isolinux.cfg
	@ echo ISO $@
	@ $(MKISOFS) -quiet -r -J -V 'PhoeniX OS' -o $@ -b isolinux.bin -c boot.cat -no-emul-boot -boot-load-size 4 -boot-info-table $(ISOROOT)/ || echo $(MKISOFS) not installed
	@ rm -rf $(ISOROOT)

.PHONY: images kernel iso

kernel: bin/phoenixos
iso: bin/phoenixos.iso

images: kernel iso
