set(LINKER_SCRIPT_SYSTEM "${CMAKE_SOURCE_DIR}/ld/system.ld")

set(KERNDIR_TAG kernel.cmake)

file(GLOB_RECURSE KERNDIRS RELATIVE ${SRCDIR} ${SRCDIR}/**/${KERNDIR_TAG})
list(TRANSFORM KERNDIRS REPLACE "/${KERNDIR_TAG}$" "")
list(TRANSFORM KERNDIRS REPLACE "/" ".")
set(KERNLIBS "")

add_compile_definitions("KERNEL_NOASLR")

foreach(kerndir ${KERNDIRS})
	file(GLOB SRCS "${SRCDIR}/${kerndir}/*.s" "${SRCDIR}/${kerndir}/*.c" "${SRCDIR}/${kerndir}/*.cpp")
  if (SRCS STREQUAL "")
    add_library(${kerndir} INTERFACE ${SRCS})
    target_include_directories(${kerndir} INTERFACE ${SRCDIR}/${kerndir}/include)
  else()
    add_library(${kerndir} STATIC ${SRCS})
    target_include_directories(${kerndir} PUBLIC ${SRCDIR}/${kerndir}/include)
    target_include_directories(${kerndir} PRIVATE ${SRCDIR}/${kerndir})
    list(APPEND KERNLIBS $<TARGET_OBJECTS:${kerndir}>)
  endif()
  include(${SRCDIR}/${kerndir}/kernel.cmake)

  file(GLOB_RECURSE INCS ${SRCDIR}/${kerndir}/include/*.h ${SRCDIR}/${kerndir}/include/*.hpp ${SRCDIR}/${kerndir}/*.h ${SRCDIR}/${kerndir}/*.hpp)
  list(APPEND LINT_SOURCES ${INCS})
  list(APPEND LINT_SOURCES ${SRCS})
endforeach(kerndir)

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
