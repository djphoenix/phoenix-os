target_link_libraries(boot PRIVATE bootinfo rand platform pagetable acpi interrupts scheduler smp syscall moduleloader)
