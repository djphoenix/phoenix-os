//    PhoeniX OS EFI Startup file
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "kernlib.hpp"
#include "efi.hpp"

namespace EFI {
  const struct EFI::SystemTable *SystemTable = 0;
  const void *ImageHandle = 0;
}
const struct EFI::SystemTable *EFI::getSystemTable() { return SystemTable; }
const void *EFI::getImageHandle() { return ImageHandle; }
