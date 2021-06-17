set(LINKER_SCRIPT_SYSTEM "${CMAKE_SOURCE_DIR}/ld/system.ld")

add_compile_definitions("KERNEL_NOASLR")

add_executable(pxkrnl.elf $<TARGET_OBJECTS:boot>)
target_link_libraries(pxkrnl.elf PRIVATE boot)
add_dependencies(pxkrnl.elf boot)

set_target_properties(pxkrnl.elf PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(pxkrnl.elf PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT_SYSTEM})
set_target_properties(
  pxkrnl.elf PROPERTIES
  LINK_FLAGS "-T ${LINKER_SCRIPT_SYSTEM} -e _start --defsym=__stack_end__=0x4000 --Bsymbolic-functions --Bsymbolic --error-unresolved-symbols --Map=${CMAKE_CURRENT_BINARY_DIR}/pxkrnl.map"
)

add_custom_target(pxkrnl ALL
  ${CMAKE_OBJCOPY} -Obinary pxkrnl.elf pxkrnl
  DEPENDS pxkrnl.elf
)
