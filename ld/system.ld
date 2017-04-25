/*    PhoeniX OS Linking script
**    Copyright © 2017 Yury Popov a.k.a. PhoeniX
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
