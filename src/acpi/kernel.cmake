target_link_libraries(acpi PUBLIC lock)
target_link_libraries(acpi PRIVATE portio bootinfo cpuid pagetable memop)
