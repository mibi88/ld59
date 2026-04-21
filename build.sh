#!/bin/sh

bdir=build

mkdir -p $bdir

[ "$cc" = "" ] && cc=cc
[ "$cflags" = "" ] && cflags="-Wall -Wextra -Wpedantic -ansi"

l=""

for i in $(find src -type f -name "*.c"); do
    echo "-- Compiling $i..."

    o=$bdir/$i.o

    mkdir -p $(dirname $o)

    $cc -c $cflags $i -o $o
    if [ $? -ne 0 ]; then
        echo "-- Build failed!"
        exit 1
    fi

    l="$l $o"
done

echo "-- Linking$l..."
$cc $l -o ld59 -lSDL -lSDL_image -lGL -lGLU -lm $ldflags
if [ $? -ne 0 ]; then
    echo "-- Build failed!"
    exit 1
fi
