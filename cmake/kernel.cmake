set(LINKER_SCRIPT_KERNEL "${CMAKE_SOURCE_DIR}/ld/kernel.ld")
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

set(PRELINK_ARGS "-nostdlib" "-static")
set(PRELINK_ARGS "${PRELINK_ARGS}" "--lto-O3" "-O3")
set(PRELINK_ARGS "${PRELINK_ARGS}" "--Bsymbolic-functions" "--Bsymbolic")
set(PRELINK_ARGS "${PRELINK_ARGS}" "-e" "_start" "--defsym=__stack_end__=0x4000" "-T" "${LINKER_SCRIPT_SYSTEM}")
add_custom_target(
  pxkrnl.prelink ALL
  ${CMAKE_LINKER} -r -o pxkrnl.prelink.obj ${PRELINK_ARGS} --start-group ${KERNLIBS} $<TARGET_FILE:modules-linked> --end-group
  BYPRODUCTS pxkrnl.prelink.obj
  DEPENDS ${LINKER_SCRIPT_SYSTEM} ${KERNDEPS} modules-linked
)
add_library(pxkrnl.single STATIC pxkrnl.prelink.obj)
set_target_properties(pxkrnl.single PROPERTIES LINKER_LANGUAGE CXX)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/null.cpp "")

add_executable(pxkrnl.elf ${CMAKE_CURRENT_BINARY_DIR}/null.cpp)
set_target_properties(pxkrnl.elf PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT_KERNEL})
set_target_properties(pxkrnl.elf PROPERTIES LINK_FLAGS "-T ${LINKER_SCRIPT_KERNEL} --Map=${CMAKE_CURRENT_BINARY_DIR}/pxkrnl.map")
target_link_libraries(pxkrnl.elf pxkrnl.single)

add_custom_target(pxkrnl ALL
  ${CMAKE_LLVM_OBJCOPY} -Obinary pxkrnl.elf pxkrnl
  DEPENDS pxkrnl.elf
)
