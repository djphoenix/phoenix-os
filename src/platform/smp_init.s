#    PhoeniX OS Application Processors bootup
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

.section .data
.global _smp_init
.global _smp_end
.code16
_smp_init:
  cli

  xor %eax, %eax
  mov %cs, %ax
  mov %ax, %ds
  shl $4, %eax
  mov %eax, %ebp
  
  add $(gd_table-_smp_init), %eax
  mov %eax, (gd_reg-_smp_init + 2)
  
  add $(_protected-gd_table), %eax
  mov %ax, (1f-_smp_init+1)
  
  lgdt gd_reg-_smp_init

  mov %cr0, %eax
  or $1, %al
  mov %eax, %cr0
  
1:
  ljmp $8, $(_protected-_smp_init)

.align 16
gd_table:
  .short 0, 0, 0, 0
  .short 0xFFFF,0x0000,0x9A00,0x00CF
  .short 0xFFFF,0x0000,0x9200,0x00CF
 
gd_reg: 
  .short .-gd_table-1
  .long gd_table - _smp_init
  
.align 16
.code32
_protected:
  mov $16, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %ss

  mov %ebp, %eax

  add $(x64_entry - _smp_init), %eax
  mov %eax, 1+1f-_smp_init(%ebp)

  mov 0+_smp_end-_smp_init(%ebp), %esi  // GDT PTR
  lgdt (%esi)

  mov %cr0, %eax
  and $0x7FFFFFFF, %eax
  mov %eax, %cr0

  mov 8+_smp_end-_smp_init(%ebp), %esi  // Pagetable ptr
  mov %esi, %cr3

  mov %cr4, %eax
  or $0x20, %eax
  mov %eax, %cr4

  mov $0xC0000080, %ecx
  rdmsr
  or $0x100, %eax
  wrmsr

  mov %cr0, %eax
  or $0x80000000, %eax
  mov %eax, %cr0

1:
  ljmpl $8, $(x64_entry-_smp_init)
  
.align 16
.code64
x64_entry:
  mov 16+_smp_end-_smp_init(%rbp), %rbx  // Local APIC address
  add $0x20, %rbx
  xor %rcx, %rcx
  mov (%rbx), %ecx
  shr $24, %rcx

  mov 24+_smp_end-_smp_init(%rbp), %rax  // CPU IDs
  xor %rdx, %rdx
1:
  cmp (%rax), %rcx
  je 2f
  add $8, %rax
  inc %rdx
  jmp 1b
2:

  shl $3, %rdx
  add 32+_smp_end-_smp_init(%rbp), %rdx  // Stacks
  mov (%rdx), %rsp

  mov 40+_smp_end-_smp_init(%rbp), %rax  // Startup
  mov %rsp, %rbp
  jmpq *%rax

.align 8
_smp_end:
