set(EFIROOT efi)
add_custom_target(${EFIROOT}
  mkdir -p ${EFIROOT}/efi/boot/
  COMMAND cp pxkrnl ${EFIROOT}/efi/boot/bootx64.efi
  DEPENDS pxkrnl
)

set(QEMU_FLAGS -smp cores=2,threads=2 -machine pc-q35-2.10 -cpu Nehalem-IBRS -serial stdio)
add_custom_target(launch
  ${QEMU} ${QEMU_FLAGS} -kernel pxkrnl
  DEPENDS pxkrnl
)
add_custom_target(launch-efi
  ${QEMU} ${QEMU_FLAGS} -drive file=fat:rw:${EFIROOT},format=raw,media=disk -bios ${OVMF_PATH}
  DEPENDS ${EFIROOT} ovmf.fd
)
add_custom_target(launch-iso
  ${QEMU} ${QEMU_FLAGS} -cdrom phoenixos.iso
  DEPENDS phoenixos.iso
)
add_custom_target(launch-iso-efi
  ${QEMU} ${QEMU_FLAGS} -cdrom phoenixos.iso -bios ${OVMF_PATH}
  DEPENDS phoenixos.iso ovmf.fd
)
