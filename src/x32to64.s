;    PhoeniX OS 32-bit mode bootup process
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

format elf64
use32
section '.text32'
extrn main
public _start
public __main
extrn grub_data
_start:
	jmp multiboot_entry

	align 4
multiboot_header:
	dd 0x1BADB002
	dd 0x00000003
	dd -(0x1BADB002 + 0x00000003)

multiboot_entry:
	cli
	
	cmp eax, 0x2BADB002
	jne .NoMultiboot

	mov [grub_data], ebx
	
	; Moving first 512K to 0x100000
	xor eax, eax
	mov ebx, 0x100000
	
.mvloop:
	mov ecx, [eax]
	mov [ebx], ecx
	add eax, 4
	add ebx, 4
	cmp eax, 0x80000
	jne .mvloop
	
	mov dx, 0x3D4
	mov al, 0xA
	out dx, al
	inc dx
	in al, dx
	or al, 0x20
	out dx, al

	mov eax, 0xB8000
._clr_loop:
	mov dword [eax], 0F000F00h
	add eax, 4
	cmp eax, 0xB8FA0
	jnz ._clr_loop
	
	mov esp, 0x2000

	pushfd						; Store the FLAGS-register.
	pop eax						; Restore the A-register.
	mov ecx, eax				; Set the C-register to the A-register.
	xor eax, 0x200000			; Flip the ID-bit, which is bit 21.
	push eax					; Store the A-register.
	popfd						; Restore the FLAGS-register.
	pushfd						; Store the FLAGS-register.
	pop eax						; Restore the A-register.
	push ecx					; Store the C-register.
	popfd						; Restore the FLAGS-register.
	xor eax, ecx				; Do a XOR-operation on the A-register and the C-register.
	jz .NoCPUID					; The zero flag is set, no CPUID.

	mov eax, 0x80000000			; Set the A-register to 0x80000000.
	cpuid						; CPU identification.
	cmp eax, 0x80000001			; Compare the A-register with 0x80000001.
	jb .NoLongMode				; It is less, there is no long mode.

	mov eax, 0x80000001			; Set the A-register to 0x80000001.
	cpuid						; CPU identification.
	test edx, 0x20000000		; Test if the LM-bit, which is bit 29, is set in the D-register.
	jz .NoLongMode				; They aren't, there is no long mode.

	mov eax, cr0				; Set the A-register to control register 0.
	and eax, 0x7FFFFFFF			; Clear the PG-bit, which is bit 31.
	mov cr0, eax				; Set control register 0 to the A-register.

	mov edi, 0x20000			; Set the destination index to 0x10000.
	mov cr3, edi				; Set control register 3 to the destination index.
	xor eax, eax				; Nullify the A-register.
	mov ecx, 7168				; Set the C-register to 7168.
	rep stosd					; Clear the memory.
	mov edi, cr3				; Set the destination index to control register 3.

	mov DWORD [edi], 0x21003		; Set the double word at the destination index to 0x2003.
	add edi, 0x1000				; Add 0x1000 to the destination index.
	mov DWORD [edi], 0x22003		; Set the double word at the destination index to 0x3003.
	add edi, 0x1000				; Add 0x1000 to the destination index.
	mov DWORD [edi+00h], 0x23003	; Set the double word at the destination index to 0x4003.
	mov DWORD [edi+08h], 0x24003	; Set the double word at the destination index to 0x5003.
	mov DWORD [edi+10h], 0x25003	; Set the double word at the destination index to 0x6003.
	mov DWORD [edi+18h], 0x26003	; Set the double word at the destination index to 0x7003.
	add edi, 0x1000				; Add 0x1000 to the destination index.

	mov ebx, 0x00000007			; Set the B-register to 0x00000007.
	mov ecx, 2048				; Set the C-register to 2048.

.SetEntry:
	mov DWORD [edi], ebx		; Set the double word at the destination index to the B-register.
	add ebx, 0x1000				; Add 0x1000 to the B-register.
	add edi, 8					; Add eight to the destination index.
	loop .SetEntry				; Set the next entry.

	mov eax, cr4				; Set the A-register to control register 4.
	or eax, 20h					; Set the PAE-bit, which is the 6th bit (bit 5).
	mov cr4, eax				; Set control register 4 to the A-register.

	mov ecx, 0xC0000080			; Set the C-register to 0xC0000080, which is the EFER MSR.
	rdmsr						; Read from the model-specific register.
	or eax, 100h				; Set the LM-bit which is the 9th bit (bit 8).
	wrmsr						; Write to the model-specific register.

	mov eax, cr0				; Set the A-register to control register 0.
	or eax, 0x80000000			; Set the PG-bit, which is the 32nd bit (bit 31).
	mov cr0, eax				; Set control register 0 to the A-register.

	lgdt [GDT64.Pointer]
	jmp (GDT64.Code - GDT64):x64_entry

.NoMultiboot:
	mov eax, aNoMultiboot
	jmp error

.NoCPUID:
	mov eax, aNoCPUID
	jmp error

.NoLongMode:
	mov eax, aNoLongMode
;	jmp error

error:
	mov edi, 0xB8000
.loop:
	mov cl, [eax]
	mov [edi], cl
	mov byte [edi+1], 0x0C
	inc eax
	add edi, 2
	test cl, cl
	jnz .loop
	jmp x64_entry.loop

x64_entry:
	call main
.loop:
	hlt
	jmp .loop

__main: ; Fix for Windows builds
	ret

section '.data'
aNoMultiboot: db "This kernel can boot only from multiboot-compatible bootloader", 0
aNoLongMode: db "Your CPU are not support x86_64 mode", 0
aNoCPUID: db "Your CPU are not support CPUID instruction", 0
	align 4

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
