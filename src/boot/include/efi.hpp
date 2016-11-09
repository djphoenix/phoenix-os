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

#define _EFIERR 0x8000000000000000LL
#define EFIAPI __attribute__((ms_abi))

enum EFI_STATUS: uint64_t {
  EFI_SUCCESS = 0,
  EFI_LOAD_ERROR = _EFIERR | 1,
  EFI_INVALID_PARAMETER = _EFIERR | 2,
  EFI_UNSUPPORTED = _EFIERR | 3,
  EFI_BAD_BUFFER_SIZE = _EFIERR | 4,
  EFI_BUFFER_TOO_SMALL = _EFIERR | 5,
  EFI_NOT_READY = _EFIERR | 6,
  EFI_DEVICE_ERROR = _EFIERR | 7,
  EFI_WRITE_PROTECTED = _EFIERR | 8,
  EFI_OUT_OF_RESOURCES = _EFIERR | 9,
  EFI_VOLUME_CORRUPTED = _EFIERR | 10,
  EFI_VOLUME_FULL = _EFIERR | 11,
  EFI_NO_MEDIA = _EFIERR | 12,
  EFI_MEDIA_CHANGED = _EFIERR | 13,
  EFI_NOT_FOUND = _EFIERR | 14,
  EFI_ACCESS_DENIED = _EFIERR | 15,
  EFI_NO_RESPONSE = _EFIERR | 16,
  EFI_NO_MAPPING = _EFIERR | 17,
  EFI_TIMEOUT = _EFIERR | 18,
  EFI_NOT_STARTED = _EFIERR | 19,
  EFI_ALREADY_STARTED = _EFIERR | 20,
  EFI_ABORTED = _EFIERR | 21,
  EFI_ICMP_ERROR = _EFIERR | 22,
  EFI_TFTP_ERROR = _EFIERR | 23,
  EFI_PROTOCOL_ERROR = _EFIERR | 24
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
  uint8_t Data4[8];
};

struct EFI_MEMORY_DESCRIPTOR {
  uint32_t Type;
  uint32_t Pad;
  uint64_t PhysicalStart;
  uint64_t VirtualStart;
  uint64_t NumberOfPages;
  uint64_t Attribute;
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
  const void *AllocatePages;
  const void *FreePages;
  const void *GetMemoryMap;
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
  const void *HandleProtocol;
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
      void *ImageHandle, uint64_t MapKey);
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
  const void *LocateProtocol;
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
  uint8_t NumberOfTableEntries;
  const EFI_CONFIGURATION_TABLE *ConfigurationTable;
};
