set(ISOROOT isodir)
set(ISONAME "PhoeniX-OS")
set(ISOOPTS -quiet -U -A ${ISONAME} -V ${ISONAME} -volset ${ISONAME} -J -joliet-long -r)
set(ISOOPTS ${ISOOPTS} -b isolinux.bin -c boot.cat -no-emul-boot -boot-load-size 4 -boot-info-table)
set(ISOOPTS ${ISOOPTS} -eltorito-alt-boot -e efiboot.img -no-emul-boot)
add_custom_target(phoenixos.iso
  mkdir -p ${ISOROOT}
	COMMAND truncate -s 4m ${ISOROOT}/efiboot.img
	COMMAND ${MKFAT} -t 1 -h 1 -s 8192 -v EFI -L 32 -i ${ISOROOT}/efiboot.img
	COMMAND ${MMD} -i ${ISOROOT}/efiboot.img ::efi
	COMMAND ${MMD} -i ${ISOROOT}/efiboot.img ::efi/boot
	COMMAND ${MCOPY} -i ${ISOROOT}/efiboot.img pxkrnl ::efi/boot/bootx64.efi
  COMMAND cp pxkrnl ${SYSLINUX_PATH}/isolinux.bin ${SYSLINUX_PATH}/ldlinux.c32 ${SYSLINUX_PATH}/libcom32.c32 ${SYSLINUX_PATH}/mboot.c32 ${ISOROOT}/
  COMMAND echo 'default /mboot.c32 /pxkrnl' > ${ISOROOT}/isolinux.cfg
  COMMAND ${MKISO} ${ISOOPTS} -o phoenixos.iso ${ISOROOT}
  DEPENDS pxkrnl syslinux
)
add_custom_target(iso DEPENDS phoenixos.iso)
