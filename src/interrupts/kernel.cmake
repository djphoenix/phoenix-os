target_link_libraries(interrupts PUBLIC kernlib)
target_link_libraries(interrupts PRIVATE acpi pagetable process printf)
