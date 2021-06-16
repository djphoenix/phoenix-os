target_link_libraries(moduleloader PUBLIC process)
target_link_libraries(moduleloader PRIVATE kernlib kernlink readelf scheduler modules-linked)
