target_link_libraries(process PUBLIC kernlib interrupts pagetable heap)
target_link_libraries(process PRIVATE syscall acpi printf)
