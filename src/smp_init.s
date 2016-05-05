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
	mov %ax, (.jmp-_smp_init+1)
	
	lgdt gd_reg-_smp_init

	mov %cr0, %eax
	or $1, %al
	mov %eax, %cr0
	
.jmp:
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
	add $(GDT64 - _smp_init), %eax
	mov %eax, 2+GDT64.Pointer-_smp_init(%ebp)
	
	add $(x64_entry - GDT64), %eax
	mov %eax, 1+.jmp64-_smp_init(%ebp)

	lgdt GDT64.Pointer-_smp_init(%ebp)

	mov %cr0, %eax
	and $0x7FFFFFFF, %eax
	mov %eax, %cr0

	mov $0x20000, %edi
	mov %edi, %cr3

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

.jmp64:
	ljmpl $8, $(x64_entry-_smp_init)
	
.align 16
.code64
x64_entry:
	mov _smp_end-_smp_init(%rbp), %rbx
	add $0x20, %rbx
	xor %rcx, %rcx
	mov (%rbx), %ecx
	shr $24, %rcx

	mov 8+_smp_end-_smp_init(%rbp), %rax
	xor %rdx, %rdx
.getid:
	cmp (%rax), %rcx
	je .foundid
	add $8, %rax
	inc %rdx
	jmp .getid
.foundid:

	shl $3, %rdx
	add 16+_smp_end-_smp_init(%rbp), %rdx
	mov (%rdx), %rsp

	mov 24+_smp_end-_smp_init(%rbp), %rax
	mov %rbp, %rsp
	callq *%rax
.loop:
	hlt
	jmp .loop


GDT64:
GDT64.Null:
	.quad 0
GDT64.Code:
	.long 0, 0x00209800
GDT64.Data:
	.long 0, 0x00009200
GDT64.Pointer:
	.short . - GDT64 - 1
	.quad GDT64 - _smp_init

_smp_end:
