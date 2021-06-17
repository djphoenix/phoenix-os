target_link_libraries(process PUBLIC pagetable heap)
target_link_libraries(process PRIVATE sprintf memop thread rand)
