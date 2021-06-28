target_link_libraries(interrupts PUBLIC lock thread)
target_link_libraries(interrupts PRIVATE sprintf portio acpi pagetable dynlist memop platform)
