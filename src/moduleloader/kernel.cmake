target_link_libraries(moduleloader PUBLIC process kernlib)
target_link_libraries(moduleloader PRIVATE kernlink readelf printf)
