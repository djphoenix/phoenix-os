target_link_libraries(kernlink PUBLIC kernlib heap)
target_link_libraries(kernlink PRIVATE syscall acpi process)
