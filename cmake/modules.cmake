set(MODDIR_TAG module.cmake)

file(GLOB_RECURSE MODDIRS RELATIVE ${SRCDIR} ${SRCDIR}/**/${MODDIR_TAG})
list(TRANSFORM MODDIRS REPLACE "/${MODDIR_TAG}$" "")

file(WRITE ${CMAKE_BINARY_DIR}/modules.s ".section .modules, \"a\"\n")
file(APPEND ${CMAKE_BINARY_DIR}/modules.s ".global __modules_start__\n__modules_start__:\n")
add_library(modules-linked STATIC ${CMAKE_BINARY_DIR}/modules.s)
foreach(moddir ${MODDIRS})
  string(REPLACE "/" "." mod ${moddir})
  set(ROOT_${mod} ${SRCDIR}/${moddir})
  file(GLOB SRCS_${mod} ${ROOT_${mod}}/*.s ${ROOT_${mod}}/*.c ${ROOT_${mod}}/*.cpp)
  add_library(mod_${mod} SHARED ${SRCS_${mod}})
  add_custom_command(
    TARGET mod_${mod} POST_BUILD
    COMMAND ${CMAKE_LLVM_OBJCOPY} --strip-non-alloc --strip-debug --strip-unneeded libmod_${mod}.so libmod_${mod}.strip.so
    COMMAND touch ${CMAKE_BINARY_DIR}/modules.s
  )
  list(APPEND LINT_SOURCES ${SRCS_${mod}})
  file(GLOB_RECURSE INCS_${mod} ${ROOT_${mod}}/*.h ${ROOT_${mod}}/*.hpp)
  list(APPEND LINT_SOURCES ${INCS_${mod}})
  add_dependencies(modules-linked mod_${mod})
  file(APPEND ${CMAKE_BINARY_DIR}/modules.s ".global __module_${mod}\n__module_${mod}:\n")
  file(APPEND ${CMAKE_BINARY_DIR}/modules.s ".incbin \"libmod_${mod}.strip.so\"\n")
endforeach(moddir)
file(APPEND ${CMAKE_BINARY_DIR}/modules.s ".global __modules_end__\n__modules_end__:\n")
