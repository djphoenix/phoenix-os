//    PhoeniX OS EFI Startup file
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "kernlib.hpp"
#include "efi.hpp"

namespace EFI {
  const struct EFI::SystemTable *SystemTable = nullptr;
  const void *ImageHandle = nullptr;
  const void *acpi1 = nullptr, *acpi2 = nullptr;
  void load();
  Framebuffer fb;
}
const struct EFI::SystemTable *EFI::getSystemTable() { load(); return SystemTable; }
const void *EFI::getImageHandle() { return ImageHandle; }

const void *EFI::getACPI1Addr() { return acpi1; }
const void *EFI::getACPI2Addr() { return acpi2; }
const struct EFI::Framebuffer *EFI::getFramebuffer() { return fb.base ? &fb : nullptr; }

void EFI::load() {
  static bool loaded = false;
  if (loaded || !SystemTable) return;

  if (SystemTable->ConfigurationTable) {
    for (uint64_t i = 0; i < SystemTable->NumberOfTableEntries; i++) {
      const EFI::ConfigurationTable *tbl = SystemTable->ConfigurationTable + i;
      if (tbl->VendorGuid == EFI::GUID_ConfigTableACPI1) acpi1 = tbl->VendorTable;
      if (tbl->VendorGuid == EFI::GUID_ConfigTableACPI2) acpi2 = tbl->VendorTable;
    }
  }
  EFI::GraphicsOutput *graphics_output = nullptr;
  SystemTable->BootServices->LocateProtocol(
      &EFI::GUID_GraphicsOutputProtocol, nullptr,
      reinterpret_cast<void**>(&graphics_output));
  if (graphics_output) {
    fb.base = graphics_output->Mode->FrameBufferBase;
    fb.width = graphics_output->Mode->Info->HorizontalResolution;
    fb.height = graphics_output->Mode->Info->VerticalResolution;
    fb.pixelFormat = graphics_output->Mode->Info->PixelFormat;
  }
  loaded = true;
}
