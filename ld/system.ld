/*    PhoeniX OS Linking script
**    Copyright © 2017 Yury Popov a.k.a. PhoeniX
*/

OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_efi_start)

PROVIDE(__stack_end__ = 0x2000);
PROVIDE(__pagetable__ = 0x20000);

PHDRS
{
  headers PT_PHDR PHDRS;
  text PT_LOAD FILEHDR;
  rodata PT_LOAD;
  data PT_LOAD;
}

SECTIONS
{
    .reloc 0x400FE0 (NOLOAD) : ALIGN(1) {
    	LONG(0) LONG(10) SHORT(0)
    } :text
    .text 0x401000 : ALIGN(1) {
        __text_start__ = . ;
        *boot.a:bootstrap.o(.text .text.*)
        *(.text .text.*)
        *(.gnu.linkonce.t.*)
        PROVIDE (etext = .);
        . = ALIGN(64);
        __text_end__ = . ;
    } :text
    .rodata : ALIGN(1) {
        . = ALIGN(8);
        *(.modules)

        . = ALIGN(8);
        __data_start__ = . ;
        *(.rodata .rodata.*)
    } :rodata
    .data : ALIGN(1) {
        . = ALIGN(8);
        __VTABLE_START__ = . ;
        __CTOR_LIST__ = . ; QUAD(0) *(.init_array*) *(.ctor*) QUAD(0)
        __DTOR_LIST__ = . ; QUAD(0) *(.dtor*) QUAD(0)
        *(.data.rel.ro.*)
        *(.data.rel.*)
        __VTABLE_END__ = . ;
        *(.got .got.*)
        *(.data .data.*)
        __data_end__ = . ;
    } :data
    .bss (NOLOAD) : ALIGN(1) {
        __bss_start__ = . ;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end__ = . ;
    } :data
    /DISCARD/ : { *(.comment) *(.eh_frame) *(.note*) }
}
