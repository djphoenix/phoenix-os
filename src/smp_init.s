;    PhoeniX OS Application Processors bootup
;    Copyright (C) 2013  PhoeniX
;
;    This program is free software: you can redistribute it and/or modify
;    it under the terms of the GNU General Public License as published by
;    the Free Software Foundation, either version 3 of the License, or
;    (at your option) any later version.
;
;    This program is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;    GNU General Public License for more details.
;
;    You should have received a copy of the GNU General Public License
;    along with this program.  If not, see <http://www.gnu.org/licenses/>.

use16
_smp_init:
	cli

	mov ax, cs
	mov ds, ax
	
	xor eax, eax
	mov ax, cs
	shl eax, 4
	
	mov [gd_reg+2], dword gd_table
	add [gd_reg+2], eax
	
	mov [.jmp+1], word _protected
	add [.jmp+1], ax
	
	lgdt [gd_reg]
	
	mov eax, cr0 
	or al, 1 
	mov cr0, eax
	
.jmp:
	jmp far 8:_protected

align 16
gd_table:
	dw 0, 0, 0, 0
	dw 0xFFFF,0x0000,0x9A00,0x00CF
	dw 0xFFFF,0x0000,0x9200,0x00CF
 
gd_reg: 
	dw gd_reg-gd_table-1
	dd gd_table
	
align 16
use32
_protected:
	mov ax, 16
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov esp, 0x1000
	call .get_eip
.get_eip:
	pop ebp
	sub ebp, .get_eip
	
	mov [ebp + GDT64.Pointer +2], dword GDT64.Null
	add [ebp + GDT64.Pointer +2], ebp
	
	mov [ebp + .jmp +1], dword x64_entry
	add [ebp + .jmp +1], ebp
	lgdt [GDT64.Pointer + ebp]

	mov eax, cr0
	and eax, 0x7FFFFFFF
	mov cr0, eax

	mov edi, 0x20000
	mov cr3, edi

	mov eax, cr4
	or eax, 20h
	mov cr4, eax

	mov ecx, 0xC0000080
	rdmsr
	or eax, 100h
	wrmsr

	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

.jmp:
	jmp (GDT64.Code - GDT64):x64_entry
	
align 16	
use64
x64_entry:
	mov rbx, [rbp + _smp_end + 0]
	add rbx, 0x20
	xor rcx, rcx
	mov ecx, [rbx]
	shr rcx, 24

    mov rax, [rbp + _smp_end + 8]
    xor rdx, rdx
_getid:
    cmp rcx, [rax]
    je _foundid
    add rax, 8
    inc rdx
    jmp _getid
_foundid:

	shl rdx, 3
	add rdx, [rbp + _smp_end + 16]
	mov rsp, [rdx]
	
	call qword [rbp + _smp_end + 24]
.loop:
	hlt
	jmp .loop


GDT64:
.Null:
	dq 0
.Code:
	dd 0
	dd 00209800h
.Data:
	dd 0
	dd 00009200h
.Pointer:
	dw $ - GDT64 - 1
	dq GDT64

_smp_end:
