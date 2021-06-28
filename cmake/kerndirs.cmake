set(KERNDIR_TAG kernel.cmake)

file(GLOB_RECURSE KERNDIRS RELATIVE ${SRCDIR} ${SRCDIR}/**/${KERNDIR_TAG})
list(TRANSFORM KERNDIRS REPLACE "/${KERNDIR_TAG}$" "")
list(TRANSFORM KERNDIRS REPLACE "/" ".")
set(KERNLIBS "")

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