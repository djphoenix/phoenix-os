ExternalProject_Add(
  ovmf.fd
  URL https://mirrors.edge.kernel.org/ubuntu/pool/universe/e/edk2/ovmf_0~20180205.c0d9813c-2_all.deb
  URL_HASH SHA256=6c73c1b99fe745d95dd5fc3e1d09924018628d40d8cbad556696f2906d335636
  EXCLUDE_FROM_ALL TRUE
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ${TAR} xf <SOURCE_DIR>/data.tar.xz -C <BINARY_DIR> --strip-components 4 ./usr/share/ovmf/OVMF.fd
  INSTALL_COMMAND ""
)
ExternalProject_Get_Property(ovmf.fd BINARY_DIR)
set(OVMF_PATH ${BINARY_DIR}/OVMF.fd)

ExternalProject_Add(
  syslinux
  URL https://mirrors.edge.kernel.org/pub/linux/utils/boot/syslinux/6.xx/syslinux-6.03.tar.xz
  URL_HASH SHA256=26d3986d2bea109d5dc0e4f8c4822a459276cf021125e8c9f23c3cca5d8c850e
  EXCLUDE_FROM_ALL TRUE
  DOWNLOAD_NO_EXTRACT ON
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ${TAR} xf <DOWNLOADED_FILE> -C <SOURCE_DIR> --strip-components 3 syslinux-6.03/bios/core/isolinux.bin syslinux-6.03/bios/com32/elflink/ldlinux/ldlinux.c32 syslinux-6.03/bios/com32/lib/libcom32.c32 syslinux-6.03/bios/com32/mboot/mboot.c32
  COMMAND cp <SOURCE_DIR>/isolinux.bin <SOURCE_DIR>/elflink/ldlinux/ldlinux.c32 <SOURCE_DIR>/lib/libcom32.c32 <SOURCE_DIR>/mboot/mboot.c32 <BINARY_DIR>/
  INSTALL_COMMAND ""
)
ExternalProject_Get_Property(syslinux BINARY_DIR)
set(SYSLINUX_PATH ${BINARY_DIR})
