set(MODDIR_TAG module.cmake)

file(GLOB_RECURSE MODDIRS RELATIVE ${SRCDIR} ${SRCDIR}/**/${MODDIR_TAG})
list(TRANSFORM MODDIRS REPLACE "/${MODDIR_TAG}$" "")

file(WRITE ${CMAKE_BINARY_DIR}/modules.s ".section .modules, \"a\"\n")
file(APPEND ${CMAKE_BINARY_DIR}/modules.s ".global __modules_start__\n__modules_start__:\n")
add_library(modules-linked OBJECT ${CMAKE_BINARY_DIR}/modules.s)
file(GLOB MODINC ${SRCDIR}/modules/include)
foreach(moddir ${MODDIRS})
  string(REPLACE "/" "." mod ${moddir})
  set(ROOT_${mod} ${SRCDIR}/${moddir})
  file(GLOB SRCS_${mod} ${ROOT_${mod}}/*.s ${ROOT_${mod}}/*.c ${ROOT_${mod}}/*.cpp)
  add_library(mod_${mod} SHARED ${SRCS_${mod}})
  target_include_directories(mod_${mod} PRIVATE ${MODINC})
  target_link_options(mod_${mod} PRIVATE "-e" "module")
  add_custom_target(mod_${mod}_strip
    ${CMAKE_LLVM_OBJCOPY} --strip-non-alloc --strip-debug --strip-unneeded libmod_${mod}.so libmod_${mod}.strip.so
    BYPRODUCTS libmod_${mod}.strip.so
    DEPENDS mod_${mod}
  )
  list(APPEND LINT_SOURCES ${SRCS_${mod}})
  file(GLOB_RECURSE INCS_${mod} ${ROOT_${mod}}/*.h ${ROOT_${mod}}/*.hpp)
  list(APPEND LINT_SOURCES ${INCS_${mod}})
  add_dependencies(modules-linked mod_${mod}_strip)
  file(APPEND ${CMAKE_BINARY_DIR}/modules.s ".global __module_${mod}\n__module_${mod}:\n")
  file(APPEND ${CMAKE_BINARY_DIR}/modules.s ".incbin \"libmod_${mod}.strip.so\"\n")
  file(APPEND ${CMAKE_BINARY_DIR}/modules.s ".size __module_${mod}, .-__module_${mod}\n")
endforeach(moddir)
file(APPEND ${CMAKE_BINARY_DIR}/modules.s ".global __modules_end__\n__modules_end__:\n")
