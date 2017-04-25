//    PhoeniX OS EFI Startup file
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "kernlib.hpp"
#include "efi.hpp"

const EFI_SYSTEM_TABLE *EFI::SystemTable = 0;
const void *EFI::ImageHandle = 0;
const EFI_SYSTEM_TABLE *EFI::getSystemTable() { return SystemTable; }
const void *EFI::getImageHandle() { return ImageHandle; }
