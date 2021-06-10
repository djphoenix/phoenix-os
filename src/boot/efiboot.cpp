//    PhoeniX OS EFI Startup file
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "kernlib.hpp"
#include "efi.hpp"

const EFI::SystemTable *EFI::systemTable = nullptr;
const void *EFI::imageHandle = nullptr;
const void *(EFI::acpi[2]) = { nullptr, nullptr };
EFI::Framebuffer EFI::fb {};

const EFI::GUID EFI::GUID_ConfigTableACPI1 { 0xEB9D2D30, 0x2D88, 0x11D3, 0x169A, 0x4DC13F279000 };
const EFI::GUID EFI::GUID_ConfigTableACPI2 { 0x8868E871, 0xE4F1, 0x11D3, 0x22BC, 0x81883CC78000 };
const EFI::GUID EFI::GUID_LoadedImageProtocol { 0x5B1B31A1, 0x9562, 0x11d2, 0x3F8E, 0x3B7269C9A000 };
const EFI::GUID EFI::GUID_GraphicsOutputProtocol { 0x9042A9DE, 0x23DC, 0x4A38, 0xFB96, 0x6A5180D0DE7A };

void EFI::load() {
  if (!systemTable) return;

  if (systemTable->ConfigurationTable) {
    for (uint64_t i = 0; i < systemTable->NumberOfTableEntries; i++) {
      const EFI::ConfigurationTable *tbl = systemTable->ConfigurationTable + i;
      if (tbl->VendorGuid == EFI::GUID_ConfigTableACPI1) acpi[0] = tbl->VendorTable;
      if (tbl->VendorGuid == EFI::GUID_ConfigTableACPI2) acpi[1] = tbl->VendorTable;
    }
  }
  EFI::GraphicsOutput *graphics_output = nullptr;
  systemTable->BootServices->LocateProtocol(
      &EFI::GUID_GraphicsOutputProtocol, nullptr,
      reinterpret_cast<void**>(&graphics_output));
  if (graphics_output) {
    fb.base = graphics_output->Mode->FrameBufferBase;
    fb.width = graphics_output->Mode->Info->HorizontalResolution;
    fb.height = graphics_output->Mode->Info->VerticalResolution;
    fb.pixelFormat = graphics_output->Mode->Info->PixelFormat;
  }
}
