cmake_minimum_required(VERSION 3.11.4)
include(ExternalProject)
include(cmake/toolchain.cmake)

project(pxos ASM C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET_PROPERTY(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)
set(SRCDIR "${CMAKE_SOURCE_DIR}/src")

include(cmake/compiler.cmake)
include(cmake/modules.cmake)
include(cmake/kernel.cmake)
include(cmake/deps.cmake)
include(cmake/iso.cmake)
include(cmake/launch.cmake)

include(cmake/cpplint.cmake)
include(cmake/pvs.cmake)
add_lint_target("${LINT_SOURCES}")
