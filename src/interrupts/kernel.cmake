target_link_libraries(interrupts PUBLIC kernlib thread)
target_link_libraries(interrupts PRIVATE acpi pagetable heap)
