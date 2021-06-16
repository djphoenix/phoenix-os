target_link_libraries(process PUBLIC kernlib pagetable heap)
target_link_libraries(process PRIVATE thread)
