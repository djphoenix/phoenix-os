//    PhoeniX OS EFI Header
//    Copyright (C) 2013  PhoeniX
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once
#include "kernlib.hpp"

#define EFIAPI __attribute__((ms_abi))

enum EFI_STATUS: uint64_t {
  EFI_SUCCESS = 0,
  EFI_ERROR = 0x8000000000000000LL,
  EFI_LOAD_ERROR = EFI_ERROR | 1,
  EFI_INVALID_PARAMETER = EFI_ERROR | 2,
  EFI_UNSUPPORTED = EFI_ERROR | 3,
  EFI_BAD_BUFFER_SIZE = EFI_ERROR | 4,
  EFI_BUFFER_TOO_SMALL = EFI_ERROR | 5,
  EFI_NOT_READY = EFI_ERROR | 6,
  EFI_DEVICE_ERROR = EFI_ERROR | 7,
  EFI_WRITE_PROTECTED = EFI_ERROR | 8,
  EFI_OUT_OF_RESOURCES = EFI_ERROR | 9,
  EFI_VOLUME_CORRUPTED = EFI_ERROR | 10,
  EFI_VOLUME_FULL = EFI_ERROR | 11,
  EFI_NO_MEDIA = EFI_ERROR | 12,
  EFI_MEDIA_CHANGED = EFI_ERROR | 13,
  EFI_NOT_FOUND = EFI_ERROR | 14,
  EFI_ACCESS_DENIED = EFI_ERROR | 15,
  EFI_NO_RESPONSE = EFI_ERROR | 16,
  EFI_NO_MAPPING = EFI_ERROR | 17,
  EFI_TIMEOUT = EFI_ERROR | 18,
  EFI_NOT_STARTED = EFI_ERROR | 19,
  EFI_ALREADY_STARTED = EFI_ERROR | 20,
  EFI_ABORTED = EFI_ERROR | 21,
  EFI_ICMP_ERROR = EFI_ERROR | 22,
  EFI_TFTP_ERROR = EFI_ERROR | 23,
  EFI_PROTOCOL_ERROR = EFI_ERROR | 24
};

static const uint64_t EFI_SYSTEM_TABLE_SIGNATURE = 0x4942492053595354LL;

enum EFI_COLOR {
  EFI_COLOR_BLACK = 0x0,
  EFI_COLOR_BLUE = 0x1,
  EFI_COLOR_GREEN = 0x2,
  EFI_COLOR_CYAN = (EFI_COLOR_BLUE | EFI_COLOR_GREEN),
  EFI_COLOR_RED = 0x4,
  EFI_COLOR_MAGENTA = (EFI_COLOR_RED | EFI_COLOR_BLUE),
  EFI_COLOR_BROWN = (EFI_COLOR_RED | EFI_COLOR_GREEN),
  EFI_COLOR_LIGHTGRAY = (EFI_COLOR_RED | EFI_COLOR_GREEN | EFI_COLOR_BLUE),

  EFI_COLOR_BRIGHT = 0x8,
  EFI_COLOR_DARKGRAY = (EFI_COLOR_BRIGHT),
  EFI_COLOR_LIGHTBLUE = (EFI_COLOR_BLUE | EFI_COLOR_BRIGHT),
  EFI_COLOR_LIGHTGREEN = (EFI_COLOR_GREEN | EFI_COLOR_BRIGHT),
  EFI_COLOR_LIGHTCYAN = (EFI_COLOR_CYAN | EFI_COLOR_BRIGHT),
  EFI_COLOR_LIGHTRED = (EFI_COLOR_RED | EFI_COLOR_BRIGHT),
  EFI_COLOR_LIGHTMAGENTA = (EFI_COLOR_MAGENTA | EFI_COLOR_BRIGHT),
  EFI_COLOR_YELLOW = (EFI_COLOR_BROWN | EFI_COLOR_BRIGHT),
  EFI_COLOR_WHITE = (EFI_COLOR_LIGHTGRAY | EFI_COLOR_BRIGHT),
};

enum EFI_RESET_TYPE {
  EfiResetCold,
  EfiResetWarm,
  EfiResetShutdown
};

enum EFI_TPL {
  EFI_TPL_APPLICATION = 4,
  EFI_TPL_CALLBACK = 8,
  EFI_TPL_NOTIFY = 16,
  EFI_TPL_HIGH_LEVEL = 31
};

enum EFI_MEMORY_TYPE: uint32_t {
  EFI_MEMORY_TYPE_RESERVED,
  EFI_MEMORY_TYPE_LOADER,
  EFI_MEMORY_TYPE_DATA,
  EFI_MEMORY_TYPE_BS_CODE,
  EFI_MEMORY_TYPE_BS_DATA,
  EFI_MEMORY_TYPE_RS_CODE,
  EFI_MEMORY_TYPE_RS_DATA,
  EFI_MEMORY_TYPE_CONVENTIONAL,
  EFI_MEMORY_TYPE_UNUSABLE,
  EFI_MEMORY_TYPE_ACPI_RECLAIM,
  EFI_MEMORY_TYPE_ACPI_NVS,
  EFI_MEMORY_TYPE_MAPPED_IO,
  EFI_MEMORY_TYPE_MAPPED_IO_PORTSPACE,
  EFI_MEMORY_TYPE_PAL_CODE
};

enum EFI_MEMORY_ATTR: uint64_t {
  EFI_MEMORY_UC = 0x01,
  EFI_MEMORY_WC = 0x02,
  EFI_MEMORY_WT = 0x04,
  EFI_MEMORY_WB = 0x08,
  EFI_MEMORY_UCE = 0x10,
  EFI_MEMORY_WP = 0x1000,
  EFI_MEMORY_RP = 0x2000,
  EFI_MEMORY_XP = 0x4000,
  EFI_MEMORY_RUNTIME = 0x8000000000000000L
};

enum EFI_ALLOCATE_TYPE {
  EFI_ALLOCATE_TYPE_ANY,
  EFI_ALLOCATE_TYPE_MAX,
  EFI_ALLOCATE_TYPE_ADDR
};

struct EFI_TABLE_HEADER {
  uint64_t Signature;
  uint32_t Revision;
  uint32_t HeaderSize;
  uint32_t CRC32;
  uint32_t Reserved;
};

struct EFI_GUID {
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint16_t Data4;
  uint64_t Data5:48;
};

struct EFI_MEMORY_DESCRIPTOR {
  EFI_MEMORY_TYPE Type;
  uint32_t Pad;
  uint64_t PhysicalStart;
  uint64_t VirtualStart;
  uint64_t NumberOfPages;
  EFI_MEMORY_ATTR Attribute;
};

union EFI_TEXT_ATTR {
  struct {
    EFI_COLOR foreground:4;
    EFI_COLOR background:4;
  } PACKED;
  uint8_t raw;
} PACKED;

struct EFI_SIMPLE_TEXT_OUTPUT_MODE {
  uint32_t MaxMode;
  uint32_t Mode;
  uint32_t Attribute;
  uint32_t CursorColumn;
  uint32_t CursorRow;
  bool CursorVisible;
};

struct EFI_TIME {
  uint16_t Year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t pad1;
  uint32_t nanosecond;
  int16_t timezone;
  uint8_t daylight;
  uint8_t pad2;
};

struct EFI_TIME_CAPABILITIES {
  uint32_t Resolution;
  uint32_t Accuracy;
  bool SetsToZero;
};

struct EFI_INPUT_KEY {
  uint16_t Scancode;
  wchar_t UnicodeChar;
};

struct EFI_SIMPLE_TEXT_OUTPUT_INTERFACE {
  EFI_STATUS EFIAPI(*const Reset)(
      const EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *self, bool ExtendedVerification);
  EFI_STATUS EFIAPI(*const OutputString)(
      const EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *self, const wchar_t *string);
  EFI_STATUS EFIAPI(*const TestString)(
      const EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *self, const wchar_t *string);
  EFI_STATUS EFIAPI(*const QueryMode)(
      const EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *self, uint64_t ModeNumber,
      uint64_t *cols, uint64_t *rows);
  EFI_STATUS EFIAPI(*const SetMode)(
      const EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *self, uint64_t ModeNumber);
  EFI_STATUS EFIAPI(*const SetAttribute)(
      const EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *self, EFI_TEXT_ATTR Attribute);
  EFI_STATUS EFIAPI(*const ClearScreen)(
      const EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *self);
  EFI_STATUS EFIAPI(*const SetCursorPosition)(
      const EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *self,
      uint64_t column, uint64_t row);
  EFI_STATUS EFIAPI(*const EnableCursor)(
      const EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *self, bool enable);
  const EFI_SIMPLE_TEXT_OUTPUT_MODE *Mode;
};

struct EFI_SIMPLE_INPUT_INTERFACE {
  EFI_STATUS EFIAPI(*const Reset)(
      const EFI_SIMPLE_INPUT_INTERFACE *self, bool ExtendedVerification);
  EFI_STATUS EFIAPI(*const ReadKeyStroke)(
      const EFI_SIMPLE_INPUT_INTERFACE *self, EFI_INPUT_KEY *key);
  const void *WaitForKey;
};

struct EFI_BOOT_SERVICES_TABLE {
  EFI_TABLE_HEADER Hdr;
  EFI_STATUS EFIAPI(*const RaisePriority)(EFI_TPL NewTpl);
  EFI_STATUS EFIAPI(*const RestorePriority)(EFI_TPL OldTpl);
  EFI_STATUS EFIAPI(*const AllocatePages)(
      EFI_ALLOCATE_TYPE Type, EFI_MEMORY_TYPE MemoryType, uint64_t NoPages,
      void **Address);
  EFI_STATUS EFIAPI(*const FreePages)(void *pages, uint64_t NoPages);
  EFI_STATUS EFIAPI(*const GetMemoryMap)(
      uint64_t *MemoryMapSize, EFI_MEMORY_DESCRIPTOR *MemoryMap,
      uint64_t *MapKey, uint64_t *DescriptorSize, uint32_t *DescriptorVersion);
  const void *AllocatePool;
  const void *FreePool;
  const void *CreateEvent;
  const void *SetTimer;
  const void *WaitForEvent;
  const void *SignalEvent;
  const void *CloseEvent;
  const void *CheckEvent;
  const void *InstallProtocolInterface;
  const void *ReInstallProtocolInterface;
  const void *UnInstallProtocolInterface;
  EFI_STATUS EFIAPI(*const HandleProtocol)(
      const void *Handle, const EFI_GUID *Protocol, void **Interface);
  const void *PCHandleProtocol;
  const void *RegisterProtocolNotify;
  const void *LocateHandle;
  const void *LocateDevicePath;
  const void *InstallConfigurationTable;
  const void *ImageLoad;
  const void *ImageStart;
  EFI_STATUS EFIAPI(*const Exit)(
      void *ImageHandle, EFI_STATUS ImageStatus,
      uint64_t ExitDataSize, const wchar_t *ExitData);
  const void *ImageUnLoad;
  EFI_STATUS EFIAPI(*const ExitBootServices)(
      const void *ImageHandle, uint64_t MapKey);
  EFI_STATUS EFIAPI(*const GetNextMonotonicCount)(uint64_t *Count);
  EFI_STATUS EFIAPI(*const Stall)(uint64_t Microseconds);
  EFI_STATUS EFIAPI(*const SetWatchdogTimer)(
      uint64_t Timeout, uint64_t WatchdogCode, uint64_t DataSize,
      const wchar_t *WatchdogData);
  const void *ConnectController;
  const void *DisConnectController;
  const void *OpenProtocol;
  const void *CloseProtocol;
  const void *OpenProtocolInformation;
  const void *ProtocolsPerHandle;
  const void *LocateHandleBuffer;
  EFI_STATUS EFIAPI(*const LocateProtocol)(
      const EFI_GUID *Protocol, const void *Registration, void **Interface);
  const void *InstallMultipleProtocolInterfaces;
  const void *UnInstallMultipleProtocolInterfaces;
  EFI_STATUS EFIAPI(*const CalculateCrc32)(
      const void *Data, uint64_t DataSize, uint32_t *Crc32);
  EFI_STATUS EFIAPI(*const CopyMem)(
      void *Destination, const void *Source, uint64_t Length);
  EFI_STATUS EFIAPI(*const SetMem)(
      void *Buffer, uint64_t Size, uint8_t Value);
  const void *CreateEventEx;
};

struct EFI_RUNTIME_SERVICES_TABLE {
  EFI_TABLE_HEADER Hdr;
  EFI_STATUS EFIAPI(*const GetTime)(
      EFI_TIME *Time, EFI_TIME_CAPABILITIES *Capabilities);
  EFI_STATUS EFIAPI(*const SetTime)(EFI_TIME *Time);
  EFI_STATUS EFIAPI(*const GetWakeUpTime)(
      bool *Enabled, bool *Pending, EFI_TIME *Time);
  EFI_STATUS EFIAPI(*const SetWakeUpTime)(bool Enabled, EFI_TIME *Time);
  EFI_STATUS EFIAPI(*const SetVirtualAddressMap)(
      uint64_t MemoryMapSize, uint64_t DescriptorSize,
      uint32_t DescriptorVersion, EFI_MEMORY_DESCRIPTOR *VirtualMap);
  EFI_STATUS EFIAPI(*const ConvertPointer)(
      uint64_t DebugDisposition, void **Address);
  EFI_STATUS EFIAPI(*const GetVariable)(
      const wchar_t *VariableName, EFI_GUID *VendorGuid, uint32_t *Attributes,
      uint64_t *DataSize, void *Data);
  EFI_STATUS EFIAPI(*const GetNextVariableName)(
      uint64_t VariableNameSize, wchar_t *VariableName, EFI_GUID *VendorGuid);
  EFI_STATUS EFIAPI(*const SetVariable)(
      const wchar_t *VariableName, EFI_GUID *VendorGuid, uint32_t Attributes,
      uint64_t DataSize, const void *Data);
  EFI_STATUS EFIAPI(*const GetNextHighMonoCount)(uint32_t *HighCount);
  EFI_STATUS EFIAPI(*const ResetSystem)(
      EFI_RESET_TYPE ResetType, EFI_STATUS ResetStatus, uint64_t DataSize,
      const wchar_t *ResetData);
};

struct EFI_CONFIGURATION_TABLE {
  EFI_GUID VendorGuid;
  const void *VendorTable;
};

static inline bool operator ==(const EFI_GUID lhs, const EFI_GUID rhs) {
  return
      lhs.Data1 == rhs.Data1 &&
      lhs.Data2 == rhs.Data2 &&
      lhs.Data3 == rhs.Data3 &&
      lhs.Data4 == rhs.Data4 &&
      lhs.Data5 == rhs.Data5;
}

static const EFI_GUID EFI_CONF_TABLE_GUID_ACPI1 =
  { 0xEB9D2D30, 0x2D88, 0x11D3, 0x169A, 0x4DC13F279000 };

static const EFI_GUID EFI_CONF_TABLE_GUID_ACPI2 =
  { 0x8868E871, 0xE4F1, 0x11D3, 0x22BC, 0x81883CC78000 };

static const EFI_GUID EFI_LOADED_IMAGE_PROTOCOL =
  { 0x5B1B31A1, 0x9562, 0x11d2, 0x3F8E, 0x3B7269C9A000 };

static const EFI_GUID EFI_GRAPHICS_OUTPUT_PROTOCOL =
  { 0x9042A9DE, 0x23DC, 0x4A38, 0xFB96, 0x6A5180D0DE7A };

struct EFI_SYSTEM_TABLE {
  EFI_TABLE_HEADER Hdr;
  const wchar_t *FirmwareVendor;
  uint32_t FirmwareRevision;
  const void *ConsoleInHandle;
  const EFI_SIMPLE_INPUT_INTERFACE *ConIn;
  const void *ConsoleOutHandle;
  const EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *ConOut;
  const void *StandardErrorHandle;
  const EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *StdErr;
  const EFI_RUNTIME_SERVICES_TABLE *RuntimeServices;
  const EFI_BOOT_SERVICES_TABLE *BootServices;
  uint64_t NumberOfTableEntries;
  const EFI_CONFIGURATION_TABLE *ConfigurationTable;
};

struct EFI_LOADED_IMAGE {
  uint32_t Revision;
  void *ParentHandle;
  EFI_SYSTEM_TABLE *SystemTable;

  void *DeviceHandle;
  wchar_t *FilePath;
  void *Reserved;

  uint32_t LoadOptionsSize;
  void *LoadOptions;

  void *ImageBase;
  uint64_t ImageSize;
  EFI_MEMORY_TYPE ImageCodeType;
  EFI_MEMORY_TYPE ImageDataType;

  EFI_STATUS EFIAPI(*const Unload)(const void *ImageHandle);
};

struct EFI_PIXEL_BITMASK {
  uint32_t RedMask;
  uint32_t GreenMask;
  uint32_t BlueMask;
  uint32_t ReservedMask;
};

enum EFI_GRAPHICS_PIXEL_FORMAT: uint32_t {
  EFI_GRAPHICS_PIXEL_FORMAT_RGBX_8BPP,
  EFI_GRAPHICS_PIXEL_FORMAT_BGRX_8BPP,
  EFI_GRAPHICS_PIXEL_FORMAT_BITMASK,
  EFI_GRAPHICS_PIXEL_FORMAT_BLTONLY
};

struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION {
  uint32_t Version;
  uint32_t HorizontalResolution;
  uint32_t VerticalResolution;
  EFI_GRAPHICS_PIXEL_FORMAT  PixelFormat;
  EFI_PIXEL_BITMASK PixelInformation;
  uint32_t PixelsPerScanLine;
};

struct EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE {
  uint32_t MaxMode;
  uint32_t Mode;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
  uint64_t SizeOfInfo;
  void *FrameBufferBase;
  uint64_t FrameBufferSize;
};

struct EFI_GRAPHICS_OUTPUT {
  EFI_STATUS EFIAPI(*const QueryMode)(
      EFI_GRAPHICS_OUTPUT *This, uint32_t ModeNumber,
      uint64_t *SizeOfInfo, EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info);
  EFI_STATUS EFIAPI(*const SetMode)(
      EFI_GRAPHICS_OUTPUT *This, uint32_t ModeNumber);
  const void *Blt;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
};

class EFI {
 private:
  static const EFI_SYSTEM_TABLE *SystemTable;
  static const void *ImageHandle;
 public:
  static const EFI_SYSTEM_TABLE *getSystemTable();
  static const void *getImageHandle();
};
