//    PhoeniX OS EFI Header
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "kernlib.hpp"

#define EFIAPI __attribute__((ms_abi))

namespace EFI {
  struct GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint16_t Data4;
    uint64_t Data5:48;
  };

  static const constexpr uint64_t SystemHandleSignature = 0x4942492053595354LL;
  static const constexpr GUID GUID_ConfigTableACPI1 = { 0xEB9D2D30, 0x2D88, 0x11D3, 0x169A, 0x4DC13F279000 };
  static const constexpr GUID GUID_ConfigTableACPI2 = { 0x8868E871, 0xE4F1, 0x11D3, 0x22BC, 0x81883CC78000 };
  static const constexpr GUID GUID_LoadedImageProtocol = { 0x5B1B31A1, 0x9562, 0x11d2, 0x3F8E, 0x3B7269C9A000 };
  static const constexpr GUID GUID_GraphicsOutputProtocol = { 0x9042A9DE, 0x23DC, 0x4A38, 0xFB96, 0x6A5180D0DE7A };

  enum Status: uint64_t {
    SUCCESS = 0,
    ERROR = 0x8000000000000000LL,
    LOAD_ERROR = ERROR | 1,
    INVALID_PARAMETER = ERROR | 2,
    UNSUPPORTED = ERROR | 3,
    BAD_BUFFER_SIZE = ERROR | 4,
    BUFFER_TOO_SMALL = ERROR | 5,
    NOT_READY = ERROR | 6,
    DEVICE_ERROR = ERROR | 7,
    WRITE_PROTECTED = ERROR | 8,
    OUT_OF_RESOURCES = ERROR | 9,
    VOLUME_CORRUPTED = ERROR | 10,
    VOLUME_FULL = ERROR | 11,
    NO_MEDIA = ERROR | 12,
    MEDIA_CHANGED = ERROR | 13,
    NOT_FOUND = ERROR | 14,
    ACCESS_DENIED = ERROR | 15,
    NO_RESPONSE = ERROR | 16,
    NO_MAPPING = ERROR | 17,
    TIMEOUT = ERROR | 18,
    NOT_STARTED = ERROR | 19,
    ALREADY_STARTED = ERROR | 20,
    ABORTED = ERROR | 21,
    ICMP_ERROR = ERROR | 22,
    TFTP_ERROR = ERROR | 23,
    PROTOCOL_ERROR = ERROR | 24
  };

  enum Color {
    COLOR_BLACK = 0x0,
    COLOR_BLUE = 0x1,
    COLOR_GREEN = 0x2,
    COLOR_CYAN = (COLOR_BLUE | COLOR_GREEN),
    COLOR_RED = 0x4,
    COLOR_MAGENTA = (COLOR_RED | COLOR_BLUE),
    COLOR_BROWN = (COLOR_RED | COLOR_GREEN),
    COLOR_LIGHTGRAY = (COLOR_RED | COLOR_GREEN | COLOR_BLUE),

    COLOR_BRIGHT = 0x8,
    COLOR_DARKGRAY = (COLOR_BRIGHT),
    COLOR_LIGHTBLUE = (COLOR_BLUE | COLOR_BRIGHT),
    COLOR_LIGHTGREEN = (COLOR_GREEN | COLOR_BRIGHT),
    COLOR_LIGHTCYAN = (COLOR_CYAN | COLOR_BRIGHT),
    COLOR_LIGHTRED = (COLOR_RED | COLOR_BRIGHT),
    COLOR_LIGHTMAGENTA = (COLOR_MAGENTA | COLOR_BRIGHT),
    COLOR_YELLOW = (COLOR_BROWN | COLOR_BRIGHT),
    COLOR_WHITE = (COLOR_LIGHTGRAY | COLOR_BRIGHT),
  };

  enum ResetType {
    RESET_COLD,
    RESET_WARM,
    RESET_SHUTDOWN
  };

  enum TPL {
    TPL_APPLICATION = 4,
    TPL_CALLBACK = 8,
    TPL_NOTIFY = 16,
    TPL_HIGH_LEVEL = 31
  };

  enum MemoryType: uint32_t {
    MEMORY_TYPE_RESERVED,
    MEMORY_TYPE_LOADER,
    MEMORY_TYPE_DATA,
    MEMORY_TYPE_BS_CODE,
    MEMORY_TYPE_BS_DATA,
    MEMORY_TYPE_RS_CODE,
    MEMORY_TYPE_RS_DATA,
    MEMORY_TYPE_CONVENTIONAL,
    MEMORY_TYPE_UNUSABLE,
    MEMORY_TYPE_ACPI_RECLAIM,
    MEMORY_TYPE_ACPI_NVS,
    MEMORY_TYPE_MAPPED_IO,
    MEMORY_TYPE_MAPPED_IO_PORTSPACE,
    MEMORY_TYPE_PAL_CODE
  };

  enum MemoryAttr: uint64_t {
    MEMORY_UC = 0x01,
    MEMORY_WC = 0x02,
    MEMORY_WT = 0x04,
    MEMORY_WB = 0x08,
    MEMORY_UCE = 0x10,
    MEMORY_WP = 0x1000,
    MEMORY_RP = 0x2000,
    MEMORY_XP = 0x4000,
    MEMORY_RUNTIME = 0x8000000000000000L
  };

  enum AllocateType {
    ALLOCATE_TYPE_ANY,
    ALLOCATE_TYPE_MAX,
    ALLOCATE_TYPE_ADDR
  };
  enum GraphicsPixelFormat: uint32_t {
    GRAPHICS_PIXEL_FORMAT_RGBX_8BPP,
    GRAPHICS_PIXEL_FORMAT_BGRX_8BPP,
    GRAPHICS_PIXEL_FORMAT_BITMASK,
    GRAPHICS_PIXEL_FORMAT_BLTONLY
  };

  struct TableHeader {
    uint64_t Signature;
    uint32_t Revision;
    uint32_t HeaderSize;
    uint32_t CRC32;
    uint32_t Reserved;
  };
  struct MemoryDescriptor {
    MemoryType Type;
    uint32_t Pad;
    uint64_t PhysicalStart;
    uint64_t VirtualStart;
    uint64_t NumberOfPages;
    MemoryAttr Attribute;
  };
  union TextAttr {
    struct {
      Color foreground:4;
      Color background:4;
    } PACKED;
    uint8_t raw;
  } PACKED;
  struct SimpleTextOutputMode {
    uint32_t MaxMode;
    uint32_t Mode;
    uint32_t Attribute;
    uint32_t CursorColumn;
    uint32_t CursorRow;
    bool CursorVisible;
  };
  struct Time {
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
  struct TimeCapabilities {
    uint32_t Resolution;
    uint32_t Accuracy;
    bool SetsToZero;
  };
  struct InputKey {
    uint16_t Scancode;
    wchar_t UnicodeChar;
  };
  struct SimpleTextOutputInterface {
    Status EFIAPI(*const Reset)(const SimpleTextOutputInterface *self, bool ExtendedVerification);
    Status EFIAPI(*const OutputString)(const SimpleTextOutputInterface *self, const wchar_t *string);
    Status EFIAPI(*const TestString)(const SimpleTextOutputInterface *self, const wchar_t *string);
    Status EFIAPI(*const QueryMode)(
        const SimpleTextOutputInterface *self, uint64_t ModeNumber, uint64_t *cols, uint64_t *rows);
    Status EFIAPI(*const SetMode)(const SimpleTextOutputInterface *self, uint64_t ModeNumber);
    Status EFIAPI(*const SetAttribute)(const SimpleTextOutputInterface *self, TextAttr Attribute);
    Status EFIAPI(*const ClearScreen)(const SimpleTextOutputInterface *self);
    Status EFIAPI(*const SetCursorPosition)(const SimpleTextOutputInterface *self, uint64_t column, uint64_t row);
    Status EFIAPI(*const EnableCursor)(const SimpleTextOutputInterface *self, bool enable);
    const SimpleTextOutputMode *Mode;
  };
  struct SimpleInputInterface {
    Status EFIAPI(*const Reset)(const SimpleInputInterface *self, bool ExtendedVerification);
    Status EFIAPI(*const ReadKeyStroke)(const SimpleInputInterface *self, InputKey *key);
    const void *WaitForKey;
  };
  struct BootServicesTable {
    TableHeader Hdr;
    Status EFIAPI(*const RaisePriority)(TPL NewTpl);
    Status EFIAPI(*const RestorePriority)(TPL OldTpl);
    Status EFIAPI(*const AllocatePages)(AllocateType Type, MemoryType MemoryType, uint64_t NoPages, void **Address);
    Status EFIAPI(*const FreePages)(void *pages, uint64_t NoPages);
    Status EFIAPI(*const GetMemoryMap)(
        uint64_t *MemoryMapSize, MemoryDescriptor *MemoryMap, uint64_t *MapKey, uint64_t *DescriptorSize,
        uint32_t *DescriptorVersion);
    Status EFIAPI(*const AllocatePool)(MemoryType MemoryType, uint64_t size, void **Address);
    Status EFIAPI(*const FreePool)(void *Address);
    const void *CreateEvent;
    const void *SetTimer;
    const void *WaitForEvent;
    const void *SignalEvent;
    const void *CloseEvent;
    const void *CheckEvent;
    const void *InstallProtocolInterface;
    const void *ReInstallProtocolInterface;
    const void *UnInstallProtocolInterface;
    Status EFIAPI(*const HandleProtocol)(const void *Handle, const GUID *Protocol, void **Interface);
    const void *PCHandleProtocol;
    const void *RegisterProtocolNotify;
    const void *LocateHandle;
    const void *LocateDevicePath;
    const void *InstallConfigurationTable;
    const void *ImageLoad;
    const void *ImageStart;
    Status EFIAPI(*const Exit)(void *ImageHandle, Status ImageStatus, uint64_t ExitDataSize, const wchar_t *ExitData);
    const void *ImageUnLoad;
    Status EFIAPI(*const ExitBootServices)(const void *ImageHandle, uint64_t MapKey);
    Status EFIAPI(*const GetNextMonotonicCount)(uint64_t *Count);
    Status EFIAPI(*const Stall)(uint64_t Microseconds);
    Status EFIAPI(*const SetWatchdogTimer)(
        uint64_t Timeout, uint64_t WatchdogCode, uint64_t DataSize, const wchar_t *WatchdogData);
    const void *ConnectController;
    const void *DisConnectController;
    const void *OpenProtocol;
    const void *CloseProtocol;
    const void *OpenProtocolInformation;
    const void *ProtocolsPerHandle;
    const void *LocateHandleBuffer;
    Status EFIAPI(*const LocateProtocol)(const GUID *Protocol, const void *Registration, void **Interface);
    const void *InstallMultipleProtocolInterfaces;
    const void *UnInstallMultipleProtocolInterfaces;
    Status EFIAPI(*const CalculateCrc32)(const void *Data, uint64_t DataSize, uint32_t *Crc32);
    Status EFIAPI(*const CopyMem)(void *Destination, const void *Source, uint64_t Length);
    Status EFIAPI(*const SetMem)(void *Buffer, uint64_t Size, uint8_t Value);
    const void *CreateEventEx;
  };
  struct RuntimeServicesTable {
    TableHeader Hdr;
    Status EFIAPI(*const GetTime)(Time *Time, TimeCapabilities *Capabilities);
    Status EFIAPI(*const SetTime)(Time *Time);
    Status EFIAPI(*const GetWakeUpTime)(bool *Enabled, bool *Pending, Time *Time);
    Status EFIAPI(*const SetWakeUpTime)(bool Enabled, Time *Time);
    Status EFIAPI(*const SetVirtualAddressMap)(
        uint64_t MemoryMapSize, uint64_t DescriptorSize, uint32_t DescriptorVersion, MemoryDescriptor *VirtualMap);
    Status EFIAPI(*const ConvertPointer)(uint64_t DebugDisposition, void **Address);
    Status EFIAPI(*const GetVariable)(
        const wchar_t *VariableName, GUID *VendorGuid, uint32_t *Attributes, uint64_t *DataSize, void *Data);
    Status EFIAPI(*const GetNextVariableName)(uint64_t VariableNameSize, wchar_t *VariableName, GUID *VendorGuid);
    Status EFIAPI(*const SetVariable)(
        const wchar_t *VariableName, GUID *VendorGuid, uint32_t Attributes, uint64_t DataSize, const void *Data);
    Status EFIAPI(*const GetNextHighMonoCount)(uint32_t *HighCount);
    Status EFIAPI(*const ResetSystem)(
        ResetType ResetType, Status ResetStatus, uint64_t DataSize, const wchar_t *ResetData);
  };
  struct ConfigurationTable {
    GUID VendorGuid;
    const void *VendorTable;
  };
  struct SystemTable {
    TableHeader Hdr;
    const wchar_t *FirmwareVendor;
    uint32_t FirmwareRevision;
    const void *ConsoleInHandle;
    const SimpleInputInterface *ConIn;
    const void *ConsoleOutHandle;
    const SimpleTextOutputInterface *ConOut;
    const void *StandardErrorHandle;
    const SimpleTextOutputInterface *StdErr;
    const RuntimeServicesTable *RuntimeServices;
    const BootServicesTable *BootServices;
    uint64_t NumberOfTableEntries;
    const ConfigurationTable *ConfigurationTable;
  };
  struct LoadedImage {
    uint32_t Revision;
    void *ParentHandle;
    SystemTable *SystemTable;

    void *DeviceHandle;
    wchar_t *FilePath;
    void *Reserved;

    uint32_t LoadOptionsSize;
    void *LoadOptions;

    void *ImageBase;
    uint64_t ImageSize;
    MemoryType ImageCodeType;
    MemoryType ImageDataType;

    Status EFIAPI(*const Unload)(const void *ImageHandle);
  };
  struct PixelBitmask {
    uint32_t RedMask;
    uint32_t GreenMask;
    uint32_t BlueMask;
    uint32_t ReservedMask;
  };
  struct GraphicsOutputModeInformation {
    uint32_t Version;
    uint32_t HorizontalResolution;
    uint32_t VerticalResolution;
    GraphicsPixelFormat  PixelFormat;
    PixelBitmask PixelInformation;
    uint32_t PixelsPerScanLine;
  };
  struct GraphicsOutputProtocolMode {
    uint32_t MaxMode;
    uint32_t Mode;
    GraphicsOutputModeInformation *Info;
    uint64_t SizeOfInfo;
    void *FrameBufferBase;
    uint64_t FrameBufferSize;
  };
  struct GraphicsOutput {
    Status EFIAPI(*const QueryMode)(
        GraphicsOutput *This, uint32_t ModeNumber, uint64_t *SizeOfInfo, GraphicsOutputModeInformation **Info);
    Status EFIAPI(*const SetMode)(GraphicsOutput *This, uint32_t ModeNumber);
    const void *Blt;
    GraphicsOutputProtocolMode *Mode;
  };
  const struct SystemTable *getSystemTable() PURE;
  const void *getImageHandle() PURE;
  const void *getACPI1Addr() PURE;
  const void *getACPI2Addr() PURE;
  struct Framebuffer {
    void *base;
    size_t width, height;
    GraphicsPixelFormat pixelFormat;
  };
  const Framebuffer *getFramebuffer() PURE;
};  // namespace EFI

static inline bool operator ==(const EFI::GUID &lhs, const EFI::GUID &rhs) {
  return
      lhs.Data1 == rhs.Data1 &&
      lhs.Data2 == rhs.Data2 &&
      lhs.Data3 == rhs.Data3 &&
      lhs.Data4 == rhs.Data4 &&
      lhs.Data5 == rhs.Data5;
}
#undef EFIAPI
