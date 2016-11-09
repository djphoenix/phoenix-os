#pragma once

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

struct EFI_TABLE_HEADER {
  uint64_t Signature;
  uint32_t Revision;
  uint32_t HeaderSize;
  uint32_t CRC32;
  uint32_t Reserved;
};

struct EFI_SIMPLE_TEXT_OUTPUT_INTERFACE;
typedef void EFIAPI (*EFI_OUTPUT_STRING)(const EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *self, const wchar_t *string);

struct EFI_SIMPLE_TEXT_OUTPUT_INTERFACE {
  void *Reset;
  const EFI_OUTPUT_STRING OutputString;
  void *TestString;
  void *QueryMode;
  void *SetMode;
  void *SetAttribute;
  void *ClearScreen;
  void *SetCursorPosition;
  void *EnableCursor;
  void *Mode;
};

struct EFI_SIMPLE_INPUT_INTERFACE {
  void *Reset;
  void *ReadKeyStroke;
  void *WaitForKey;
};

struct EFI_BOOT_SERVICES_TABLE {
  EFI_TABLE_HEADER Hdr;
  void *RaisePriority;
  void *RestorePriority;
  void *AllocatePages;
  void *FreePages;
  void *GetMemoryMap;
  void *AllocatePool;
  void *FreePool;
  void *CreateEvent;
  void *SetTimer;
  void *WaitForEvent;
  void *SignalEvent;
  void *CloseEvent;
  void *CheckEvent;
  void *InstallProtocolInterface;
  void *ReInstallProtocolInterface;
  void *UnInstallProtocolInterface;
  void *HandleProtocol;
  void *Void;
  void *RegisterProtocolNotify;
  void *LocateHandle;
  void *LocateDevicePath;
  void *InstallConfigurationTable;
  void *ImageLoad;
  void *ImageStart;
  void *Exit;
  void *ImageUnLoad;
  void *ExitBootServices;
  void *GetNextMonotonicCount;
  void *Stall;
  void *SetWatchdogTimer;
  void *ConnectController;
  void *DisConnectController;
  void *OpenProtocol;
  void *CloseProtocol;
  void *OpenProtocolInformation;
  void *ProtocolsPerHandle;
  void *LocateHandleBuffer;
  void *LocateProtocol;
  void *InstallMultipleProtocolInterfaces;
  void *UnInstallMultipleProtocolInterfaces;
  void *CalculateCrc32;
  void *CopyMem;
  void *SetMem;
};

struct EFI_RUNTIME_SERVICES_TABLE {
  EFI_TABLE_HEADER Hdr;
  void *GetTime;
  void *SetTime;
  void *GetWakeUpTime;
  void *SetWakeUpTime;
  void *SetVirtualAddressMap;
  void *ConvertPointer;
  void *GetVariable;
  void *GetNextVariableName;
  void *SetVariable;
  void *GetNextHighMonoCount;
  void *ResetSystem;
};

struct EFI_SYSTEM_TABLE {
  EFI_TABLE_HEADER Hdr;
  wchar_t *FirmwareVendor;
  uint32_t FirmwareRevision;
  void *ConsoleInHandle;
  EFI_SIMPLE_INPUT_INTERFACE *ConIn;
  void *ConsoleOutHandle;
  EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *ConOut;
  void *StandardErrorHandle;
  EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *StdErr;
  EFI_RUNTIME_SERVICES_TABLE *RuntimeServices;
  EFI_BOOT_SERVICES_TABLE *BootServices;
  uint8_t NumberOfTableEntries;
  void *ConfigurationTable;
};
