/*    PhoeniX OS Linking script
**    Copyright © 2017 Yury Popov a.k.a. PhoeniX
*/

OUTPUT_FORMAT(elf64-x86-64)

SECTIONS
{
    .text 0x400000 : ALIGN(0x1000) {
        *(.text .text.*)
    }
    .rodata ALIGN(0x1000) : ALIGN(0x1000) {
        *(.rodata .rodata.*)
    }
    .data ALIGN(0x1000) : ALIGN(0x1000) {
        *(.data .data.*)
        *(.got .got.*)
    }
    .bss (NOLOAD) : ALIGN(8) {
        __bss_start__ = . ;
        *(.bss .bss.*)
        __bss_end__ = . ;
    }
    /DISCARD/ : { *(.comment) *(.eh_frame) *(.note*) }
}
