@echo off
for /f %%i in ("%0") do set curpath=%%~dpi
cd /d %curpath%
set CFLAGS=-c -nostdlib -s -m64 -O2 -fno-leading-underscore
set BIN=bin/pxkrnl

echo === Building kernel ===

md obj
for %%i in ("src\*.s") do call:as %%~ni
for %%i in ("src\*.cpp") do call:cc %%~ni
call:link
call:oc
goto:eof

:cc
  echo  [CC]	%~1
  x86_64-w64-mingw32-gcc %CFLAGS% src/%~1.cpp -o obj/%~1.o
  set OBJS=%OBJS% obj/%~1.o
goto:eof

:as
  echo  [ASM]	%~1
  fasm src/%~1.s obj/%~1.o >nul
  set OBJS=%OBJS% obj/%~1.o
goto:eof

:link
  echo  [LD]	%BIN%
  x86_64-w64-mingw32-ld -belf64-x86-64 -o %BIN% -s --nostdlib %OBJS%
goto:eof

:oc
  echo  [COPY]	%BIN%
  x86_64-w64-mingw32-objcopy %BIN% -Felf32-i386
goto:eof
