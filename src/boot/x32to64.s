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
.extern
.extern main
.global _start
.global __main
.global _efi_start
.global GDT64
.global GDT64_PTR
.extern multiboot
.extern __text_start__
.extern __modules_end__
.extern __stack_end__
.extern __pagetable__
.extern __bss_start__
.extern __bss_end__
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

  cld

  # Clear BSS
  mov $__bss_start__, %edi
  xor %ecx, %ecx
  mov $__bss_end__, %ecx
  sub $__bss_start__, %ecx
  xor %eax, %eax
  shr $2, %ecx
  rep stosl

  # Advance pointer to multiboot table if needed
  cmp $0x80000, %ebx
  jge 1f
  add $__bss_end__, %ebx
1:
  mov %ebx, multiboot

  # Moving first 512K to BSS
  xor %esi, %esi
  mov $__bss_end__, %edi
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

  mov $__stack_end__, %esp

  pushfl                    # Store the FLAGS-register.
  pop %eax                  # Restore the A-register.
  mov %eax, %ecx            # Set the C-register to the A-register.
  xor $0x200000, %eax       # Flip the ID-bit, which is bit 21.
  push %eax                 # Store the A-register.
  popfl                     # Restore the FLAGS-register.
  pushfl                    # Store the FLAGS-register.
  pop %eax                  # Restore the A-register.
  push %ecx                 # Store the C-register.
  popfl                     # Restore the FLAGS-register.
  xor %ecx, %eax            # Do a XOR-operation on the A-register and the C-register.
  jz 3f                     # The zero flag is set, no CPUID.

  mov $0x80000000, %eax     # Set the A-register to 0x80000000.
  cpuid                     # CPU identification.
  cmp $0x80000001, %eax     # Compare the A-register with 0x80000001.
  jb 4f                     # It is less, there is no long mode.

  mov $0x80000001, %eax     # Set the A-register to 0x80000001.
  cpuid                     # CPU identification.
  test $0x20000000, %edx    # Test if the LM-bit, which is bit 29, is set in the D-register.
  jz 4f                     # They aren't, there is no long mode.

  mov %cr0, %eax            # Set the A-register to control register 0.
  and $0x7FFFFFFF, %eax     # Clear the PG-bit, which is bit 31.
  mov %eax, %cr0            # Set control register 0 to the A-register.

  mov $__pagetable__, %edi  # Set the destination index to 0x10000.
  mov %edi, %cr3            # Set control register 3 to the destination index.
  xor %eax, %eax            # Nullify the A-register.
  mov $0x1C00, %ecx         # Set the C-register to 7168.
  rep stosl                 # Clear the memory.
  mov %cr3, %edi            # Set the destination index to control register 3.

  movl $__pagetable__ + 0x1003, (%edi)      # Set the double word at the destination index to 0x2003.
  add $0x1000, %edi                         # Add 0x1000 to the destination index.
  movl $__pagetable__ + 0x2003, (%edi)      # Set the double word at the destination index to 0x3003.
  add $0x1000, %edi                         # Add 0x1000 to the destination index.
  movl $__pagetable__ + 0x3003, 0x00(%edi)  # Set the double word at the destination index to 0x4003.
  movl $__pagetable__ + 0x4003, 0x08(%edi)  # Set the double word at the destination index to 0x5003.
  movl $__pagetable__ + 0x5003, 0x10(%edi)  # Set the double word at the destination index to 0x6003.
  movl $__pagetable__ + 0x6003, 0x18(%edi)  # Set the double word at the destination index to 0x7003.
  add $0x1000, %edi       # Add 0x1000 to the destination index.

  mov $0x00000007, %ebx   # Set the B-register to 0x00000007.
  mov $0x800, %ecx        # Set the C-register to 2048.

1:
  mov %ebx, (%edi)        # Set the double word at the destination index to the B-register.
  add $0x1000, %ebx       # Add 0x1000 to the B-register.
  add $8, %edi            # Add eight to the destination index.
  loop 1b          # Set the next entry.

  mov %cr4, %eax          # Set the A-register to control register 4.
  or $0x20, %eax          # Set the PAE-bit, which is the 6th bit (bit 5).
  mov %eax, %cr4          # Set control register 4 to the A-register.

  mov $0xC0000080, %ecx   # Set the C-register to 0xC0000080, which is the EFER MSR.
  rdmsr                   # Read from the model-specific register.
  or $0x100, %eax         # Set the LM-bit which is the 9th bit (bit 8).
  wrmsr                   # Write to the model-specific register.

  mov %cr0, %eax          # Set the A-register to control register 0.
  or $0x80000000, %eax    # Set the PG-bit, which is the 32nd bit (bit 31).
  mov %eax, %cr0          # Set control register 0 to the A-register.

  lgdt GDT64.Pointer
  mov $16, %ax
  mov %ax, %ss
  ljmp $8, $x64_entry

2:
  mov $aNoMultiboot, %eax
  jmp 5f

3:
  mov $aNoCPUID, %eax
  jmp 5f

4:
  mov $aNoLongMode, %eax
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
  mov %rsp, %rbp
  jmp main

__main: # Fix for Windows builds
  ret
  
  .data

aNoMultiboot: .ascii "This kernel can boot only from multiboot-compatible bootloader\0"
aNoLongMode: .ascii "Your CPU are not support x86_64 mode\0"
aNoCPUID: .ascii "Your CPU are not support CPUID instruction\0"
  .align 4

GDT64_PTR:
GDT64.Pointer:
  .short GDT64 - GDT64.End - 1
  .quad GDT64

GDT64:
GDT64.Null:
  .quad 0
GDT64.Code:
  .long 0, 0x00209800
GDT64.Data:
  .long 0, 0x00009200
GDT64.End:

  .section .reloc, "a"
  .long 0
  .long 10
  .word 0
