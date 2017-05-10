For launch PhoeniX OS on PC you need to:

* Get binary kernel [from CI](https://git.phoenix.dj/phoenix/phoenix-os/builds/artifacts/master/download?job=build) or [build it from source](build.md)
* Create bootable [CD](#cd) or [USB](#usb)
* Boot your PC using created media

## CD
For launch from CD you need to create ISO image with bootloader and PhoeniX OS kernel, then build it to blank CD.

For build ISO image you may use these tools:

* Linux
    * Brasero (gnume / Ubuntu)
    * K3b (KDE / KUbubntu)
* [Mac OS X](https://support.apple.com/kb/HT2087?viewlocale=en_US) (documentation at https://support.apple.com/)
* Windows XP
    * [CDBurnerXP](https://cdburnerxp.se/?lang=en) ([documentation](https://cdburnerxp.se/help/Data/burn-iso) on official website)
* [Windows 7](http://windows.microsoft.com/en-us/windows7/burn-a-cd-or-dvd-from-an-iso-file) (or later) (documentation at http://windows.microsoft.com/)


### Prepare files
Download and extract archive of latest [SYSLINUX binaries](https://www.kernel.org/pub/linux/utils/boot/syslinux/syslinux-6.03.zip) from [Kernel.org](https://kernel.org/).

Create a folder on hard drive that contains:

1. `phoenixos` - PhoeniX OS kernel file
2. `isolinux.bin` from SYSLINUX archive (`bios/core/isolinux.bin`)
3. `ldlinux.c32` from SYSLINUX archive (`bios/com32/elflink/ldlinux/ldlinux.c32`)
4. `libcom32.c32` from SYSLINUX archive (`bios/com32/lib/libcom32.c32`)
5. `mboot.c32` from SYSLINUX archive (`bios/com32/mboot/mboot.c32`)
6. `isolinux.cfg` - isolinux [bootloader configuration](http://www.syslinux.org/wiki/index.php?title=Config) (text file) contains `default /mboot.c32 /phoenixos`

### Linux / Mac OS X
For creation of ISO image used `mkisofs` tool contained in `cdrtools` package. Install it with command:

* Mac OS X: `brew install cdrtools` ([Homebrew](http://brew.sh/) required)
* Linux (debian/ubuntu): `sudo apt-get install cdrtools`

Then (in terminal) switch working directory (`cd`) to folder that contains files from step "Prepare files" and run command

    mkisofs -o ../phoenixos.iso -b isolinux.bin -c boot.cat -no-emul-boot -boot-load-size 4 -boot-info-table .

In result, in parent folder, will be created file `phoenixos.iso` that you should write on CD.

### Windows
Download and install [`CDR-Tools`](http://cdrtoolswin.sourceforge.net/) ([direct download link](http://sourceforge.net/projects/cdrtoolswin/files/1.0/Binaries/CDR-Tools.exe/download)).

Place [this file](https://git.phoenix.dj/snippets/3) into folder that contains files from step "Prepare files" and run it.

In result, in parent folder, will be created file `phoenixos.iso` that you should write on CD.

## USB
TODO