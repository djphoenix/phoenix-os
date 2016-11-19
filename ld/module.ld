/*    PhoeniX OS Linking script
**    Copyright (C) 2013  PhoeniX
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

OUTPUT_FORMAT(elf64-x86-64)
ENTRY(module)
SECTIONS
{
    .text BLOCK(1) : ALIGN(1) {
        *(.init)
        *(.text .text.*)
        *(.gnu.linkonce.t.*)
        *(.fini)
    }
    .rodata BLOCK(1) : ALIGN(1) {
        *(.init_array*)
        *(.rodata .rodata.* .rdata .rdata.*)
    }
    .module BLOCK(1) : ALIGN(1) {
        *(.module*)
    }
    .data BLOCK(1) : ALIGN(1) {
        *(.data .data.*)
        *(.xdata .xdata.* .pdata .pdata.*)
    }
    .bss BLOCK(1) : ALIGN(1) {
        *(.bss .bss.*)
        *(COMMON)
    }
    /DISCARD/ : { *(.comment) *(.eh_frame) *(.note*) }
}
