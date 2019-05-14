//    PhoeniX OS EFI Startup file
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "kernlib.hpp"
#include "efi.hpp"

namespace EFI {
  const struct EFI::SystemTable *SystemTable = nullptr;
  const void *ImageHandle = nullptr;
}
const struct EFI::SystemTable *EFI::getSystemTable() { return SystemTable; }
const void *EFI::getImageHandle() { return ImageHandle; }
