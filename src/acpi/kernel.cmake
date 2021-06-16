target_link_libraries(acpi PUBLIC kernlib)
target_link_libraries(acpi PRIVATE bootinfo cpuid pagetable)
