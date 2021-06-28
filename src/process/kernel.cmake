target_link_libraries(process PUBLIC pagetable heap dynlist)
target_link_libraries(process PRIVATE sprintf memop thread rand)
