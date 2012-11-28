cd `dirname $0`
CFLAGS="-c -nostdlib -s -m64 -O2"
BIN="bin/pxkrnl"

function cc {
  echo "CC   $1"
  gcc -m64 $CFLAGS src/$1.cpp -o obj/$1.o > /dev/null
  OBJS="$OBJS obj/$1.o"
}

function as {
  echo "ASM  $1"
  fasm src/$1.s obj/$1.o > /dev/null
  OBJS="$OBJS obj/$1.o"
}

function link {
  echo "LD   $BIN"
  ld -o $BIN -s -melf_x86_64 --nostdlib $OBJS > /dev/null
}

function oc {
  echo "COPY $BIN"
  objcopy $BIN -Felf32-i386
}

echo === Building kernel ===
rm -rf obj
mkdir obj
as x32to64
cc boot
cc pxlib
cc memory
link
rm -rf obj
oc