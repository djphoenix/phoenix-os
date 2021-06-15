target_link_libraries(scheduler PUBLIC process interrupts)
target_link_libraries(scheduler PRIVATE heap acpi)
