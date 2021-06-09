#    PhoeniX OS Application Processors bootup
#    Copyright © 2017 Yury Popov a.k.a. PhoeniX

.section .rodata
.global _smp_init
.global _smp_end
.code16
_smp_init:
  cli
  
  # Fix data segment & set base pointer
  xor %ebp, %ebp
  mov %cs, %bp
  mov %bp, %ds
  shl $4, %ebp

  # Fix entry point addr
  lea (x64_smp-_smp_init)(%ebp), %eax
  mov %eax, (1f-_smp_init+2)

  # Load pagetable
  mov _smp_end-_smp_init+0, %edx
  mov %edx, %cr3
  # Load GDT
  lgdt _smp_end-_smp_init+40
  
  # Enable PAE & PGE
  mov %cr4, %eax
  or $0x6B0, %eax
  mov %eax, %cr4
  
  # Enable LME
  mov $0xC0000080, %ecx
  rdmsr
  or $0x900, %eax
  wrmsr
  
  # Enable PG & PE
  mov %cr0, %eax
  and $0xFFFB, %ax
  or $0x80000003, %eax
  mov %eax, %cr0

  # Jump to 64-bit mode
1:ljmpl $8, $(x64_smp-_smp_init)

.type _smp_init, function
.size _smp_init, .-_smp_init

.align 4
.code64
x64_smp:
  # Load params
  add $_smp_end-_smp_init, %bp
  mov  8(%rbp), %rbx  # Local APIC address
  mov 16(%rbp), %rax  # CPU IDs
  mov 24(%rbp), %rcx  # Stacks
  mov 32(%rbp), %rdx  # Startup

  # Get local APIC ID
  xor %r8, %r8
  mov 0x20(%rbx), %r8d
  shr $24, %r8

  # Find CPU ID
  xor %r9, %r9
1:
  cmp (%rax,%r9,8), %r8
  je 2f
  inc %r9
  jmp 1b
2:

  # Load stack pointer
  mov (%rcx,%r9,8), %rsp

  # Jump back to SMP initializer
  xor %rbp, %rbp
  jmpq *%rdx
.type x64_smp, function
.size x64_smp, .-x64_smp

.align 8
_smp_end:
