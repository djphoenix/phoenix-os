#    PhoeniX OS 32-bit mode bootup process
#    Copyright © 2017 Yury Popov a.k.a. PhoeniX

.code32
.section .text
.global _start
.global __main
.global _efi_start
_start:
  jmp multiboot_entry

  .align 4
multiboot_header:
  .long 0x1BADB002
  .long 0x00010005
  .long -(0x1BADB002+0x00010005)

  .long multiboot_header
  .long __text_start__
  .long __bss_start__
  .long __bss_end__ + 0x80000
  .long multiboot_entry

  .long 0, 800, 600, 8

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
  lea __bss_end__-_start(%ebp), %ecx
  sub %edi, %ecx
  xor %eax, %eax
  rep stosb

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

.code64

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

x64_entry:
  and $(~0x0F), %rsp
  call reloc_vtables
  xor %rbp, %rbp
  call _ZN4RAND5setupEv
  call _ZN9Pagetable4initEv
  call _ZN7Display5setupEv
  call _ZN10Interrupts4initEv
  call _ZN3SMP4initEv
  call _ZN13ModuleManager4initEv
  call _ZN7Syscall5setupEv
  jmp _ZN14ProcessManager12process_loopEv

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

static_init:
  push %rbp
  push %rax
  lea __CTOR_LIST__(%rip), %rbp
1:
  add $8, %rbp
  mov (%rbp), %rax
  test %rax, %rax
  je 2f
  callq *%rax
  jmp 1b
2:
  pop %rax
  pop %rbp
  ret

__main: # Fix for Windows builds
  ret

  .rodata

aNoMultiboot: .ascii "This kernel can boot only from multiboot-compatible bootloader\0"
aNoLongMode: .ascii "Your CPU is not support 64-bit mode\0"

.align 16
GDT64.Pointer:
  .short GDT64.End - GDT64 - 1
  .quad GDT64

.align 16
GDT64:
GDT64.Null:
  .quad 0
GDT64.Code:
  .quad 0x00AF9A000000FFFF
GDT64.Data:
  .quad 0x00CF92000000FFFF
GDT64.End:
