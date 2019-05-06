set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_CROSSCOMPILING ON)

set(CROSS_LLVM_PATH /usr/local/opt/llvm/bin)
set(CROSS_BINUTILS_PATH /usr/local/opt/binutils/bin)

find_program(CMAKE_C_COMPILER clang HINTS ${CROSS_LLVM_PATH})
find_program(CMAKE_CXX_COMPILER clang++ HINTS ${CROSS_LLVM_PATH})

find_program(CMAKE_LLVM_OBJCOPY llvm-objcopy HINTS ${CROSS_LLVM_PATH})
find_program(CMAKE_OBJCOPY objcopy HINTS ${CROSS_BINUTILS_PATH})
find_program(CMAKE_AR llvm-ar HINTS ${CROSS_LLVM_PATH})
find_program(CMAKE_NM llvm-nm HINTS ${CROSS_LLVM_PATH})
find_program(CMAKE_RANLIB llvm-ranlib HINTS ${CROSS_LLVM_PATH})
find_program(CMAKE_LINKER ld.lld HINTS ${CROSS_LLVM_PATH})

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set(CROSS_LINKER_FLAGS "")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER> -o <TARGET> <LINK_FLAGS> <OBJECTS> <LINK_LIBRARIES>")
set(CMAKE_CXX_CREATE_SHARED_LIBRARY "<CMAKE_LINKER> --shared -o <TARGET> <LINK_FLAGS> <OBJECTS> <LINK_LIBRARIES>")

set(CMAKE_EXE_LINKER_FLAGS "${CROSS_LINKER_FLAGS} -g -nostdlib -O2 --hash-style=sysv" CACHE STRING "" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS "${CROSS_LINKER_FLAGS} -g -nostdlib -O2 --hash-style=sysv" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS "${CROSS_LINKER_FLAGS} -g -nostdlib -O2 --hash-style=sysv" CACHE STRING "" FORCE)
set(CMAKE_STATIC_LINKER_FLAGS ${CROSS_LINKER_FLAGS} CACHE STRING "" FORCE)

set(CROSS_TARGET x86_64-none-elf)
set(CROSS_COMPILER_FLAGS "")

set(CMAKE_ASM_COMPILER_TARGET ${CROSS_TARGET})
set(CMAKE_C_COMPILER_TARGET ${CROSS_TARGET})
set(CMAKE_CXX_COMPILER_TARGET ${CROSS_TARGET})

set(CMAKE_C_FLAGS "${CROSS_COMPILER_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64 -mno-sse")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions -fno-rtti -fshort-wchar")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O2 -Os")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections -fpic -fpie")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wproperty-attribute-mismatch")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-qual -Wcast-align -Winline")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "" FORCE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")

set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> qc <TARGET> <OBJECTS>")
set(CMAKE_CXX_ARCHIVE_CREATE "${CMAKE_C_ARCHIVE_CREATE}")

find_program(TAR tar)
find_program(MKISO xorriso)
if(MKISO)
	set(MKISO ${MKISO} -as mkisofs)
else()
  find_program(MKISO mkisofs)
endif()

find_program(MKFAT mformat)
find_program(MCOPY mcopy)
find_program(MMD mmd)
find_program(QEMU qemu-system-x86_64)

project(stub)
SET_PROPERTY(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)
