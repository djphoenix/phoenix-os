target_link_libraries(pagetable PUBLIC bootinfo lock)
target_link_libraries(pagetable PRIVATE sprintf rand memop platform)
