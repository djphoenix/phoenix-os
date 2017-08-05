define DOWN_RULE
deps/$(DEP_$(1)_FILE):
	@ mkdir -p $$(dir $$@)
	$(QECHO) DL $$@
	$(Q) wget -q '$(DEP_$(1)_URL)' -O $$@
	@ touch $$@
endef

DEP_LIST := SYSLINUX OVMF

DEP_SYSLINUX_VER = 6.03
DEP_SYSLINUX_URL = https://www.kernel.org/pub/linux/utils/boot/syslinux/6.xx/syslinux-6.03.zip
DEP_SYSLINUX_FILE = syslinux-$(DEP_SYSLINUX_VER).zip

DEP_OVMF_VER = 20161202
DEP_OVMF_URL = http://mirrors.kernel.org/ubuntu/pool/universe/e/edk2/ovmf_0~20161202.7bbe0b3e-1_all.deb
DEP_OVMF_FILE = ovmf-$(DEP_OVMF_VER).deb

$(foreach d,$(DEP_LIST),$(eval $(call DOWN_RULE,$(d))))
