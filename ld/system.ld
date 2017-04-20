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
ENTRY(_efi_start)

PROVIDE(__stack_start__ = 0x1000);
PROVIDE(__stack_end__ = 0x2000);
PROVIDE(__first_page__ = 0x2000);
PROVIDE(__pagetable__ = 0x20000);

SECTIONS
{
    .reloc 0x400FE0 : ALIGN(1) {}
    .text 0x401000 : ALIGN(1) {
        __text_start__ = . ;
        *boot.a:bootstrap.o(.text .text.*)
        *(.text .text.*)
        *(.gnu.linkonce.t.*)
        PROVIDE (etext = .);
        . = ALIGN(64);
        __text_end__ = . ;
    }
    .data : ALIGN(1) {
        __data_start__ = . ;
        *(.rodata .rodata.*)
        . = ALIGN(8);
        __VTABLE_START__ = . ;
        __INIT_LIST__ = . ; QUAD(0) *(.init_array*) QUAD(0)
        __CTOR_LIST__ = . ; QUAD(0) *(.ctor*) QUAD(0)
        __DTOR_LIST__ = . ; QUAD(0) *(.dtor*) QUAD(0)
        *(.data.rel.*)
        __VTABLE_END__ = . ;
        *(.got .got.*)
        *(.data .data.*)
        __data_end__ = . ;

        __modules_start__ = . ;
        *(.modules)
        . = ALIGN(64);
        __modules_end__ = . ;
    }
    .bss : ALIGN(1) {
        __bss_start__ = . ;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end__ = . ;
    }
    /DISCARD/ : { *(.comment) *(.eh_frame) *(.note*) }
}
