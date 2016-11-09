//    PhoeniX OS Startup file
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
  uint64_t EFIAPI efi_main(void*, EFI_SYSTEM_TABLE*);
}

uint64_t EFIAPI efi_main(void*, EFI_SYSTEM_TABLE *ST) {
  const wchar_t hello[] = L"PhoeniX OS: EFI mode boot\r\n";
  ST->ConOut->OutputString(ST->ConOut, hello);
  for (;;) {}
  return EFI_SUCCESS;
}
