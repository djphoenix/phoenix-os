target_link_libraries(process PUBLIC pagetable heap)
target_link_libraries(process PRIVATE kernlib thread rand)
