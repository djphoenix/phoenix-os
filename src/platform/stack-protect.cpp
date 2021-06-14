#include <stdint.h>

extern "C" {
  __attribute__((used)) uintptr_t __stack_chk_guard = 0xee737e43f1908c23;
  __attribute__((noreturn)) void __stack_chk_fail(void) {
    asm volatile("ud2; 1: hlt; jmp 1b");
    for (;;);
  }
}
