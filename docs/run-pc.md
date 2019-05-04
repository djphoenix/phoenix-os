For launch PhoeniX OS on PC you need to:

* Get binary kernel [from CI](https://git.phoenix.dj/phoenix/phoenix-os/builds/artifacts/master/download?job=build) or [build it from source](build.md)
* Create bootable [CD](#cd) or [USB](#usb)
* Boot your PC using created media

## CD
For launch from CD you need to create ISO image with bootloader and PhoeniX OS kernel, then build it to blank CD.

To make bootable ISO you may use `make iso` command in source directory (see [build](build.md) section).

For build ISO image you may use these tools:

* Linux
    * Brasero (gnume / Ubuntu)
    * K3b (KDE / KUbubntu)
* [Mac OS X](https://support.apple.com/kb/HT2087?viewlocale=en_US) (documentation at https://support.apple.com/)
* Windows XP
    * [CDBurnerXP](https://cdburnerxp.se/?lang=en) ([documentation](https://cdburnerxp.se/help/Data/burn-iso) on official website)
* [Windows 7](http://windows.microsoft.com/en-us/windows7/burn-a-cd-or-dvd-from-an-iso-file) (or later) (documentation at http://windows.microsoft.com/)

## USB
TODO