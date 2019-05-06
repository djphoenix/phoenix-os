set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/ld/system.ld")

set(KERNDIR_TAG kernel.cmake)

file(GLOB_RECURSE KERNDIRS RELATIVE ${SRCDIR} ${SRCDIR}/**/${KERNDIR_TAG})
list(TRANSFORM KERNDIRS REPLACE "/${KERNDIR_TAG}$" "")
list(TRANSFORM KERNDIRS REPLACE "/" ".")

foreach(kerndir ${KERNDIRS})
	list (APPEND INCDIRS "${SRCDIR}/${kerndir}/include")
endforeach(kerndir)

list(REMOVE_ITEM KERNDIRS "boot")

foreach(kerndir ${KERNDIRS})
	file(GLOB SRCS "${SRCDIR}/${kerndir}/*.s" "${SRCDIR}/${kerndir}/*.c" "${SRCDIR}/${kerndir}/*.cpp")
  add_library(${kerndir} STATIC ${SRCS})
  file(GLOB_RECURSE INCS ${SRCDIR}/${kerndir}/include/*.h ${SRCDIR}/${kerndir}/include/*.hpp)
  list(APPEND LINT_SOURCES ${INCS})
  list(APPEND LINT_SOURCES ${SRCS})
  target_include_directories(${kerndir} PRIVATE ${INCDIRS})
endforeach(kerndir)

file(GLOB kern_SRCS "${SRCDIR}/boot/*.s" "${SRCDIR}/boot/*.c" "${SRCDIR}/boot/*.cpp")
add_executable(pxkrnl.elf ${kern_SRCS})
file(GLOB_RECURSE kern_INCS ${SRCDIR}/boot/include/*.h ${SRCDIR}/boot/include/*.hpp)
list(APPEND LINT_SOURCES ${kern_INCS})
list(APPEND LINT_SOURCES ${kern_SRCS})
target_include_directories(pxkrnl.elf PRIVATE ${INCDIRS})

set_target_properties(pxkrnl.elf PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})
set_target_properties(pxkrnl.elf PROPERTIES LINK_FLAGS "-T ${LINKER_SCRIPT}")
target_link_libraries(pxkrnl.elf ${KERNDIRS} modules-linked)
add_custom_target(pxkrnl ALL
  ${CMAKE_LLVM_OBJCOPY} --strip-all pxkrnl.elf pxkrnl.elf.strip
  COMMAND ${CMAKE_OBJCOPY} -Opei-x86-64 --subsystem efi-app --file-alignment 1 --section-alignment 1 pxkrnl.elf.strip pxkrnl
  DEPENDS pxkrnl.elf
)
