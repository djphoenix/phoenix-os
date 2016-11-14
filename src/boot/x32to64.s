#    PhoeniX OS 32-bit mode bootup process
#    Copyright (C) 2013  PhoeniX
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http:#www.gnu.org/licenses/>.

.code32
.section .text32
.extern main
.global _start
.global __main
.global _efi_start
.global GDT64
.global GDT64_PTR
_start:
  jmp multiboot_entry

  .align 4
multiboot_header:
  .long 0x1BADB002
  .long 0x00010003
  .long -(0x1BADB002+0x00010003)

  .long multiboot_header
  .long __text_start__
  .long __modules_end__
  .long __bss_end__ + 0x80000
  .long multiboot_entry


multiboot_entry:
  cli
  
  cmp $0x2BADB002, %eax
  jne 2f

  call 1f
1:
  pop %ebp
  sub $1b - _start, %ebp
  
  cld

  # Clear BSS
  lea __bss_start__-_start(%ebp), %edi
  xor %ecx, %ecx
  lea __bss_end__-_start(%ebp), %ecx
  sub %edi, %ecx
  xor %eax, %eax
  shr $2, %ecx
  rep stosl

  # Advance pointer to multiboot table if needed
  lea __bss_end__-_start(%ebp), %edi
  cmp $0x80000, %ebx
  jge 1f
  add %edi, %ebx
1:
  mov %ebx, multiboot-_start(%ebp)

  # Moving first 512K to BSS
  xor %esi, %esi
  lea __bss_end__-_start(%ebp), %edi
  mov $0x20000, %ecx
  rep movsl

  # Clear screen
  mov $0xB8000, %edi
  mov $0x0F000F00, %eax
  mov $1000, %ecx
  rep stosl

  mov $0x3D4, %dx
  mov $0xA, %al
  out %al, %dx
  inc %dx
  in %dx, %al
  or $0x20, %al
  out %al, %dx

  lea __stack_end__, %esp

  # Check CPUID
  pushfl
  pop %eax
  mov %eax, %ecx
  xor $0x200000, %eax
  push %eax
  popfl
  pushfl
  pop %eax
  push %ecx
  popfl
  xor %ecx, %eax
  jz 3f

  # Check extended CPUID is supported
  mov $0x80000000, %eax
  cpuid
  cmp $0x80000001, %eax
  jb 4f

  # Check longmode in CPUID
  mov $0x80000001, %eax
  cpuid
  test $0x20000000, %edx
  jz 4f

  # Disable paging
  mov %cr0, %eax
  and $0x7FFFFFFF, %eax
  mov %eax, %cr0

  # Fill pagetable
  lea __pagetable__-_start(%ebp), %edi
  mov %edi, %cr3
  xor %eax, %eax
  mov $0x1C00, %ecx
  rep stosl
  mov %cr3, %edi

  lea __pagetable__-_start+0x1007(%ebp), %esi
                     mov %esi, 0x0000(%edi)
  add $0x1000, %esi; mov %esi, 0x1000(%edi)
  add $0x1000, %esi; mov %esi, 0x2000(%edi)
  add $0x1000, %esi; mov %esi, 0x2008(%edi)
  add $0x1000, %esi; mov %esi, 0x2010(%edi)
  add $0x1000, %esi; mov %esi, 0x2018(%edi)
  
  add $0x3000, %edi

  mov $0x00000007, %ebx
  mov $0x800, %ecx
1:
  mov %ebx, (%edi)
  add $0x1000, %ebx
  add $8, %edi
  loop 1b

  # Enable PAE
  mov %cr4, %eax
  or $0x20, %eax
  mov %eax, %cr4

  # Enable longmode
  mov $0xC0000080, %ecx
  rdmsr
  or $0x100, %eax
  wrmsr

  # Enable pading
  mov %cr0, %eax
  or $0x80000000, %eax
  mov %eax, %cr0

  # Load GDT
  lea GDT64.Pointer-_start(%ebp), %eax
  lgdt (%eax)

  # Load stack segment
  mov $16, %ax
  mov %ax, %ss

  # Fix 64-bit entry point
  lea x64_entry-_start(%ebp), %eax
  mov %eax, 1f+1
1:
  ljmp $8, $x64_entry

2:
  lea aNoMultiboot-_start(%ebp), %eax
  jmp 5f

3:
  lea aNoCPUID-_start(%ebp), %eax
  jmp 5f

4:
  lea aNoLongMode-_start(%ebp), %eax
#  jmp 5f

5:
  mov $0xB8000, %edi
6:
  mov (%eax), %cl
  mov %cl, (%edi)
  movb $0x0C, 1(%edi)
  inc %eax
  add $2, %edi
  test %cl, %cl
  jnz 6b
7:
  hlt
  jmp 7b
  
.code64

_efi_start: # EFI
  jmp efi_main

x64_entry:
  call reloc_vtables
  call static_init
  mov %rsp, %rbp
  jmp main
  
reloc_vtables:
  lea _start(%rip), %rcx
  sub $_start, %rcx
  lea __VTABLE_START__(%rip), %rbp
  lea __VTABLE_END__(%rip), %rdx
1:
  cmp %rbp, %rdx
  je 3f
  cmp $0, (%rbp)
  je 2f
  add %rcx, (%rbp)
2:
  add $8, %rbp
  jmp 1b
3:
  ret

static_init:
  push %rbp
  push %rax
  lea __INIT_LIST__(%rip), %rbp
1:
  add $8, %rbp
  mov (%rbp), %rax
  test %rax, %rax
  je 2f
  callq *%rax
  jmp 1b
2:
  lea __CTOR_LIST__(%rip), %rbp
3:
  add $8, %rbp
  mov (%rbp), %rax
  test %rax, %rax
  je 4f
  callq *%rax
  jmp 3b
4:
  pop %rax
  pop %rbp
  ret  

__main: # Fix for Windows builds
  ret
  
  .data

aNoMultiboot: .ascii "This kernel can boot only from multiboot-compatible bootloader\0"
aNoLongMode: .ascii "Your CPU are not support x86_64 mode\0"
aNoCPUID: .ascii "Your CPU are not support CPUID instruction\0"
  .align 4

GDT64_PTR:
GDT64.Pointer:
  .short GDT64.End - GDT64 - 1
  .quad GDT64

GDT64:
GDT64.Null:
  .quad 0
GDT64.Code:
  .quad 0x00AF9A000000FFFF
GDT64.Data:
  .quad 0x00CF92000000FFFF
GDT64.End:

  .section .reloc, "a"
  .long 0
  .long 10
  .word 0
