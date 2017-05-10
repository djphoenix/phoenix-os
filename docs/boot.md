## Bootup process
0. Kernel load through any [multiboot](multiboot.md)-compatible bootloader (GRUB / SYSLINUX etc) or via [onboard EFI](efi.md) directly
1. Switch CPU to [long mode](https://en.wikipedia.org/wiki/Long_mode). If CPU is incompatible, prints error message.
2. Jumps to C kernel code

## Initialization
1. Initialize static variables
2. Clears screen
3. Initialize [display](display.md)
3. Initialize [memory subsystem](memory.md)
4. Initialize [SMP](https://en.wikipedia.org/wiki/Symmetric_multiprocessing)
5. Initialize [interrupts manager](interrupts.md)
6. Initialize [module manager](modules.md)
7. Start [process manager](process-manager.md)