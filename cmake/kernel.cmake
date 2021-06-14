set(LINKER_SCRIPT_SYSTEM "${CMAKE_SOURCE_DIR}/ld/system.ld")

set(KERNDIR_TAG kernel.cmake)

file(GLOB_RECURSE KERNDIRS RELATIVE ${SRCDIR} ${SRCDIR}/**/${KERNDIR_TAG})
list(TRANSFORM KERNDIRS REPLACE "/${KERNDIR_TAG}$" "")
list(TRANSFORM KERNDIRS REPLACE "/" ".")
set(KERNLIBS "")

add_compile_definitions("KERNEL_NOASLR")

foreach(kerndir ${KERNDIRS})
	list (APPEND INCDIRS "${SRCDIR}/${kerndir}/include")
endforeach(kerndir)

foreach(kerndir ${KERNDIRS})
	file(GLOB SRCS "${SRCDIR}/${kerndir}/*.s" "${SRCDIR}/${kerndir}/*.c" "${SRCDIR}/${kerndir}/*.cpp")
  file(GLOB_RECURSE INCS ${SRCDIR}/${kerndir}/include/*.h ${SRCDIR}/${kerndir}/include/*.hpp)
  add_library(${kerndir} STATIC ${SRCS})
  target_include_directories(${kerndir} PRIVATE ${INCDIRS})
  list(APPEND KERNDEPS ${kerndir})
  list(APPEND KERNLIBS $<TARGET_FILE:${kerndir}>)
  list(APPEND LINT_SOURCES ${INCS})
  list(APPEND LINT_SOURCES ${SRCS})
endforeach(kerndir)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/null.cpp "")

add_executable(pxkrnl.elf ${CMAKE_CURRENT_BINARY_DIR}/null.cpp)
target_link_libraries(pxkrnl.elf --start-group ${KERNLIBS} $<TARGET_OBJECTS:modules-linked> --end-group)
add_dependencies(pxkrnl.elf modules-linked)

set_target_properties(pxkrnl.elf PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT_SYSTEM})
set_target_properties(
  pxkrnl.elf PROPERTIES
  LINK_FLAGS "-T ${LINKER_SCRIPT_SYSTEM} -e _start --defsym=__stack_end__=0x4000 --Bsymbolic-functions --Bsymbolic --error-unresolved-symbols --Map=${CMAKE_CURRENT_BINARY_DIR}/pxkrnl.map"
)

add_custom_target(pxkrnl ALL
  ${CMAKE_LLVM_OBJCOPY} -Obinary pxkrnl.elf pxkrnl
  DEPENDS pxkrnl.elf
)
