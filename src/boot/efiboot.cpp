//    PhoeniX OS EFI Startup file
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

#include "kernlib.hpp"
#include "efi.hpp"

extern "C" {
  EFI_STATUS EFIAPI efi_main(void*, EFI_SYSTEM_TABLE*);
}

EFI_SYSTEM_TABLE *ST;

static inline void Print(const char *string) {
  size_t len = strlen(string) + 1;
  wchar_t *buf = reinterpret_cast<wchar_t*>(alloca(len * sizeof(wchar_t)));
  for (size_t i = 0; i < len; i++) {
    buf[i] = string[i];
  }
  ST->ConOut->OutputString(ST->ConOut, buf);
}

static inline void PrintFormat(const char *string, ...) {
  va_list va, tva;
  va_start(va, string);
  va_copy(tva, va);
  size_t len = vsnprintf(0, 0, string, tva);
  va_end(tva);
  char *buf = reinterpret_cast<char*>(alloca(len));
  vsnprintf(buf, len, string, va);
  va_end(va);
  Print(buf);
}

static inline void SetupConsole() {
  ST->ConOut->Reset(ST->ConOut, 1);
  ST->ConOut->SetAttribute(
      ST->ConOut,
      { {
         .foreground = EFI_COLOR_WHITE,
         .background = EFI_COLOR_BLACK
      } });

  const uint32_t maxmode = ST->ConOut->Mode->MaxMode;
  uint32_t bestmode = 0, bestrows = 0, bestcols = 0;
  uint64_t rows = 0, cols = 0;
  for (uint32_t mode = 0; mode <= maxmode; mode++) {
    ST->ConOut->QueryMode(ST->ConOut, mode, &cols, &rows);
    if (cols > bestcols || rows > bestrows) {
      bestcols = cols;
      bestrows = rows;
      bestmode = mode;
    }
  }
  ST->ConOut->SetMode(ST->ConOut, bestmode);
}

static inline void MemoryMap() {
  uint64_t size = 0;
  EFI_MEMORY_DESCRIPTOR *map = 0;
  uint64_t key = 0;
  uint64_t desc_size = 0;
  uint32_t ver = 0;
  ST->BootServices->GetMemoryMap(&size, map, &key, &desc_size, &ver);
  map = reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(alloca(size));
  ST->BootServices->GetMemoryMap(&size, map, &key, &desc_size, &ver);
  for (EFI_MEMORY_DESCRIPTOR *ent = map;
      ent < reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>((uintptr_t)map + size);
      ent = reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>((uintptr_t)ent + desc_size)
          ) {
    if (ent->Type == EFI_MEMORY_TYPE_CONVENTIONAL) continue;
    if ((ent->Attribute & EFI_MEMORY_RUNTIME) != 0) continue;
    PrintFormat("% 16s %#010lx-%#010lx -> %#010lx [%lX]\r\n",
                EFI_MEMORY_TYPE_STR[ent->Type],
                ent->PhysicalStart,
                ent->PhysicalStart + (ent->NumberOfPages*4096),
                ent->VirtualStart, ent->Attribute);
  }
}

EFI_STATUS EFIAPI efi_main(void*, EFI_SYSTEM_TABLE *SystemTable) {
  ST = SystemTable;
  SetupConsole();
  Print("PhoeniX OS: EFI mode boot\r\n");
  MemoryMap();

  void *pt;
  asm("mov %%cr3, %q0":"=r"(pt));
  PrintFormat("Pagetable: %p\r\n", pt);
  void *base;
  asm("lea __text_start__(%%rip), %q0":"=r"(base));
  PrintFormat("Base: %p\r\n", base);
  for (;;) {}
  return EFI_SUCCESS;
}
