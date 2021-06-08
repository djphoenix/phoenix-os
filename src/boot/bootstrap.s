#    PhoeniX OS 32-bit mode bootup process
#    Copyright © 2017 Yury Popov a.k.a. PhoeniX

.global _start
.global __main
.global _efi_start
.global x64_entry

.code32

.section .text.bs32
_start:
  .short 0x5a4d /* MZ Signature */
  .short 0x0000 /* MZ Extra bytes */
  .short 0x0000 /* MZ Pages */
  .short 0x0000 /* MZ Relocation items */

  .short 0x0000 /* MZ Header size */
  .short 0x0000 /* MZ Minimum allocation */
  .short 0xFFFF /* MZ Maximum allocation */
  .short 0x0000 /* MZ Initial SS */

  .short 0x00B8 /* MZ Initial SP */
  .short 0x0000 /* MZ Checksum */
  .short 0x0000 /* MZ Initial IP */
  .short 0x0000 /* MZ Initial CS */

  .short 0x0000 /* MZ Relocation table */
  .short 0x0000 /* MZ Overlay */

  .quad  0 /* MZ-PE Reserved */
  .short 0 /* MZ-PE OEM identifier */
  .short 0 /* MZ-PE OEM info */
  .long  0 /* MZ-PE Reserved 20 bytes */
  .long  0 /* ... */
  .long  0 /* ... */
  .long  0 /* ... */
  .long  0 /* ... */

  .long  (pe_header - _start)

pe_header:
  .long  0x00004550 /* PE Magic */
  .short 0x8664 /* PE Machine */
  .short (pe_header_sect_end-pe_header_sect)/40 /* PE NumberOfSections */
  .long  0 /* PE TimeDateStamp */
  .long  0 /* PE PointerToSymbolTable */
  .long  1 /* PE NumberOfSymbols */
  .short (pe_header_sect - pe_header_opt) /* PE SizeOfOptionalHeader */
  .short (0x0002 | 0x0200 | 0x0004) /* PE Characteristics */

pe_header_opt:
  .short 0x020b /* PE64 Magic */
  .byte  0x02, 0x14 /* PE64 MajorLinkerVersion, MinorLinkerVersion */
  .long  _sect_text_size-(multiboot_entry-_start) /* PE64 SizeOfCode */
  .long  _full_data_size /* PE64 SizeOfInitializedData */
  .long  _sect_bss_size /* PE64 SizeOfUninitializedData */

  .long  _efi_start-_start /* PE64 AddressOfEntryPoint */
  .long  0 /* PE64 BaseOfCode */

  .quad  _start /* PE64 ImageBase */
  .long  0x1000 /* PE64 SectionAlignment */
  .long  0x1000 /* PE64 FileAlignment */

  .short 1 /* PE64 MajorOperatingSystemVersion */
  .short 0 /* PE64 MinorOperatingSystemVersion */
  .short 0 /* PE64 MajorImageVersion */
  .short 0 /* PE64 MinorImageVersion */

  .short 0 /* PE64 MajorSubsystemVersion */
  .short 0 /* PE64 MinorSubsystemVersion */
  .long  0 /* PE64 Win32VersionValue */

  .long  __data_end__-_start /* PE64 SizeOfImage */
  .long  (pe_header_sect_end-_start) /* PE64 SizeOfHeaders */

  .long  0 /* PE64 CheckSum */
  .short 10 /* PE64 Subsystem */
  .short 0 /* PE64 DllCharacteristics */

  .quad  0 /* PE64 SizeOfStackReserve */
  .quad  0 /* PE64 SizeOfStackCommit */
  .quad  0 /* PE64 SizeOfHeapReserve */
  .quad  0 /* PE64 SizeOfHeapCommit */

  .long  0 /* PE64 LoaderFlags */
  .long  (pe_header_sect-.) / 8 /* PE64 NumberOfRvaAndSizes */

  .quad  0 /* ExportTable */
  .quad  0 /* ImportTable */
  .quad  0 /* ResourceTable */
  .quad  0 /* ExceptionTable */
  .quad  0 /* CertificationTable */
  .long  pe_reloc-_start, pe_reloc_end-pe_reloc /* BaseRelocationTable */
  .quad  0 /* DebugDirectory */
  .quad  0 /* ArchitectureSpecificData */
  .quad  0 /* RVAofGP */
  .quad  0 /* TLSDirectory */
  .quad  0 /* LoadConfigurationDirectory */
  .quad  0 /* BoundImportDirectoryinheaders */
  .quad  0 /* ImportAddressTable */
  .quad  0 /* DelayLoadImportDescriptors */
  .quad  0 /* COMRuntimedescriptor */
  .quad  0 /* EOD */

pe_header_sect:
  .ascii	".reloc\0\0"
  .long	pe_reloc_end-pe_reloc
  .long	pe_reloc-_start
  .long	pe_reloc_end-pe_reloc
  .long	pe_reloc-_start
	.long	0, 0    # PointerToRelocations, PointerToLineNumbers
	.word	0, 0    # NumberOfRelocations, NumberOfLineNumbers
  .long	0x00000040 | 0x40000000 | 0x02000000 | 0x00100000 # Characteristics

/*  .ascii	".setup\0\0"
  .long	0
  .long	0x0     # startup_{32,64}
  .long	0       # Size of initialized data on disk
  .long	0x0     # startup_{32,64}
  .long	0       # PointerToRelocations
  .long	0       # PointerToLineNumbers
  .word	0       # NumberOfRelocations
  .word	0       # NumberOfLineNumbers
  .long	0x00000020 | 0x40000000 | 0x20000000 | 0x00500000 # Characteristics */

  .ascii	".text\0\0\0"
	.long	_sect_text_size-(multiboot_entry-_start)
	.long	multiboot_entry-_start
	.long	_sect_text_size-(multiboot_entry-_start)
	.long	multiboot_entry-_start
	.long	0, 0    # PointerToRelocations, PointerToLineNumbers
	.word	0, 0    # NumberOfRelocations, NumberOfLineNumbers
	.long	0x00000020 | 0x40000000 | 0x20000000 | 0x00d00000 # Characteristics

  .ascii	".rodata\0"
	.long	_sect_rodata_size
	.long	__rodata_start__-_start
	.long	_sect_rodata_size
	.long	__rodata_start__-_start
	.long	0, 0    # PointerToRelocations, PointerToLineNumbers
	.word	0, 0    # NumberOfRelocations, NumberOfLineNumbers
	.long	0x00000040 | 0x40000000 | 0x00d00000 # Characteristics

  .ascii	".data\0\0\0"
	.long	_sect_data_size
	.long	__data_start__-_start
	.long	_sect_data_size
	.long	__data_start__-_start
	.long	0, 0    # PointerToRelocations, PointerToLineNumbers
	.word	0, 0    # NumberOfRelocations, NumberOfLineNumbers
	.long	0x00000040 | 0x40000000 | 0x80000000 | 0x00d00000 # Characteristics

  .ascii	".bss\0\0\0\0"
	.long	_sect_bss_size
	.long	__bss_start__-_start
	.long	0
	.long	0
	.long	0, 0    # PointerToRelocations, PointerToLineNumbers
	.word	0, 0    # NumberOfRelocations, NumberOfLineNumbers
	.long	0x00000080 | 0x40000000 | 0x80000000 | 0x00500000 # Characteristics
pe_header_sect_end:
  .size pe_header_sect, .-pe_header_sect
  .size pe_header, .-pe_header

pe_reloc:
  .short 0
  .long 0, 0
pe_reloc_end:
  .size pe_reloc, .-pe_reloc

  .align 16
multiboot_header:
  .long 0x1BADB002
  .long 0x00010005
  .long -(0x1BADB002+0x00010005)

  .long multiboot_header
  .long __text_start__
  .long __data_end__
  .long __bss_end__ + 0x80000
  .long multiboot_entry

  .long 0, 800, 600, 8
.size multiboot_header, .-multiboot_header

  .align 0x1000
multiboot_entry:
  cli

  # Load stack pointer
  lea __stack_end__, %esp

  # Check for multiboot
  cmp $0x2BADB002, %eax
  jne 2f

  # Save current value on stack
  mov -4(%esp), %eax

  # Get base pointer
  call 1f
1:
  pop %ebp
  sub $1b - _start, %ebp

  # Fix stack value
  mov %eax, -4(%esp)

  cld

  # Clear BSS
  lea __bss_start__-_start(%ebp), %edi
  mov $_sect_bss_size, %ecx
  shr $2, %ecx
  xor %eax, %eax
  rep stosl

  # Advance pointer to multiboot table if needed
  cmp $0x80000, %ebx
  jge 1f
  add %edi, %ebx
1:
  # Multiboot::payload
  mov %ebx, _ZN9Multiboot7payloadE-_start(%ebp)

  # Moving first 512K to BSS
  xor %esi, %esi
  mov $0x20000, %ecx
  rep movsl

  # Clear screen
  mov $0xB8000, %edi
  xor %eax, %eax
  mov $1000, %ecx
  rep stosl

  # Disable cursor
  mov $0x3D4, %dx
  mov $0xA, %al
  out %al, %dx
  inc %dx
  in %dx, %al
  or $0x20, %al
  out %al, %dx

  # Check longmode in CPUID
  mov $0x80000001, %eax
  cpuid
  test $0x20000000, %edx
  jz 3f

  # Disable paging
  mov %cr0, %eax
  and $0x7FFFFFFF, %eax
  mov %eax, %cr0

  # Fill pagetable
  lea -0x203000(%ebp), %edi
  mov %edi, %cr3
  xor %eax, %eax
  mov $0x400, %ecx
  rep stosl
  mov %cr3, %edi

  lea (-0x202000+0x3)(%ebp), %eax

  mov %eax, (%edi)
  add $0x1000, %eax
  add $0x1000, %edi
  mov %eax, (%edi)
  add $0x1000, %edi

  mov $0x200, %ecx
1:
  add $0x1000, %eax
  mov %eax, (%edi)
  add $8, %edi
  loop 1b

  mov $0x00000003, %eax
  mov $0x8000, %ecx
1:
  mov %eax, (%edi)
  add $0x1000, %eax
  add $8, %edi
  loop 1b

  # Enable PAE, PSE & SSE
  mov %cr4, %eax
  or $0x630, %ax
  mov %eax, %cr4

  # Enable longmode
  mov $0xC0000080, %ecx
  rdmsr
  or $0x901, %eax
  wrmsr

  # Enable paging & SSE
  mov %cr0, %eax
  and $0xFFFB, %ax
  or $0x80000002, %eax
  mov %eax, %cr0

  # Load GDT
  lgdt GDT64.Pointer-_start(%ebp)

  # Load stack segment
  mov $16, %ax
  mov %ax, %ss

  # Fix 64-bit entry point
  lea x64_entry-_start(%ebp), %eax
  mov %eax, 1f+1
1:
  ljmp $8, $x64_entry

2:
  lea aNoMultiboot-_start(%ebp), %esi
  jmp 4f
3:
  lea aNoLongMode-_start(%ebp), %esi
#  jmp 4f

4:
  mov $0xB8000, %edi
  mov $0x0C00, %ax
5:
  mov (%esi), %al
  stosw
  inc %esi
  test %al, %al
  jnz 5b
6:
  hlt
  jmp 6b

.type multiboot_entry, function
.size multiboot_entry, .-multiboot_entry

.code64
.section .text.bs64

_efi_start: # EFI
  cli
  # EFI::SystemTable
  mov %rdx, _ZN3EFI11SystemTableE(%rip)
  # EFI::ImageHandle
  mov %rcx, _ZN3EFI11ImageHandleE(%rip)
  # Enable NX
  mov $0xC0000080, %rcx
  rdmsr
  or $0x801, %rax
  wrmsr
  # Enable SSE
  mov %cr0, %rax
  and $0xFFFB, %ax
  or $0x02, %ax
  mov %rax, %cr0
  mov %cr4, %rax
  or $0x600, %ax
  mov %rax, %cr4
.type _efi_start, function
.size _efi_start, .-_efi_start

x64_entry:
  and $(~0x0F), %rsp
  call reloc_vtables
  xor %rbp, %rbp
  call _ZN4RAND5setupEv
  call _ZN13SerialConsole5setupEv
  call _ZN9Pagetable4initEv
  call _ZN10Interrupts4initEv
  call _ZN3SMP4initEv
  call _ZN13ModuleManager4initEv
  call _ZN7Syscall5setupEv
  jmp _ZN14ProcessManager12process_loopEv
.type x64_entry, function
.size x64_entry, .-x64_entry

.section .text.reloc_vtables
reloc_vtables:
  lea reloc_vtables-reloc_vtables(%rip), %rcx
  lea __VTABLE_START__(%rip), %rbp
  lea __VTABLE_END__(%rip), %rdx
  lea __text_start__, %rax
  lea __bss_end__, %r8
1:
  cmp %rbp, %rdx
  je 3f
  cmpq %rax, (%rbp)
  jl 2f
  cmpq %r8, (%rbp)
  jg 2f
  add %rcx, (%rbp)
2:
  add $8, %rbp
  jmp 1b
3:
  ret
.type reloc_vtables, function
.size reloc_vtables, .-reloc_vtables

.section .text.__main
__main: # Fix for Windows builds
  ret
.type __main, function
.size __main, .-__main

  .rodata

aNoMultiboot:
  .ascii "This kernel can boot only from multiboot-compatible bootloader\0"
.size aNoMultiboot, .-aNoMultiboot

aNoLongMode:
  .ascii "Your CPU is not support 64-bit mode\0"
.size aNoLongMode, .-aNoLongMode

.align 16
GDT64.Pointer:
  .short GDT64.End - GDT64 - 1
  .quad GDT64
.size GDT64.Pointer, .-GDT64.Pointer

.align 16
GDT64:
  # Null
  .quad 0
  # Code
  .quad 0x00AF9A000000FFFF
  # Data
  .quad 0x00CF92000000FFFF
GDT64.End:
.size GDT64, .-GDT64
