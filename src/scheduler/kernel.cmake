target_link_libraries(scheduler PUBLIC heap interrupts)
target_link_libraries(scheduler PRIVATE process acpi)
