target_link_libraries(kernlink PUBLIC heap)
target_link_libraries(kernlink PRIVATE kernlib syscall acpi process interrupts)
