/*    PhoeniX OS Linking script
**    Copyright © 2017 Yury Popov a.k.a. PhoeniX
*/

OUTPUT_FORMAT(elf64-x86-64)

SECTIONS
{
    .text 0x400000 : ALIGN(0x1000) {
        __text_start__ = . ;
        KEEP(*(.text.efi_head))
        KEEP(*(.text.bs32))
        KEEP(*(.text.bs64))
        *(.text .text.*)
        . = ALIGN(8);
        __text_end__ = . ;

        PROVIDE(_sect_text_size = (__text_end__ - __text_start__));
    }
    .rodata : ALIGN(0x1000) {
        __rodata_start__ = . ;
        *(.modules)
        . = ALIGN(8);
        *(.rodata .rodata.*)
        . = ALIGN(8);
        __rodata_end__ = . ;

        PROVIDE(_sect_rodata_size = (__rodata_end__ - __rodata_start__));
    }
    .data : ALIGN(0x1000) {
        __data_start__ = . ;
        __VTABLE_START__ = . ;
        *(.data.rel.ro.* .data.rel.*)
        __VTABLE_END__ = . ;
        *(.got .got.*)
        *(.data .data.*)
        . = ALIGN(8);
        __data_end__ = . ;

        PROVIDE(_sect_data_size = (__data_end__ - __data_start__));
        PROVIDE(_full_data_size = (__data_end__ - __rodata_start__));
    }
    .bss (NOLOAD) : ALIGN(8) {
        __bss_start__ = . ;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(8);
        __bss_end__ = . ;

        PROVIDE(_sect_bss_size = (__bss_end__ - __bss_start__));
    }
    /DISCARD/ : { *(.comment) *(.eh_frame) *(.note*) }
}
