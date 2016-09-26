DEP_SYSLINUX_VER = 6.03
DEP_SYSLINUX_URL = https://www.kernel.org/pub/linux/utils/boot/syslinux/6.xx/syslinux-6.03.zip

DEP_OVMF_VER = r15214
DEP_OVMF_URL = http://netcologne.dl.sourceforge.net/project/edk2/OVMF/OVMF-X64-r15214.zip

deps/syslinux-$(DEP_SYSLINUX_VER).zip:
	@ mkdir -p $(dir $@)
	@ echo DL $@
	@ wget -q '$(DEP_SYSLINUX_URL)' -O $@

deps/ovmf-$(DEP_OVMF_VER).zip:
	@ mkdir -p $(dir $@)
	@ echo DL $@
	@ wget -q '$(DEP_OVMF_URL)' -O $@
