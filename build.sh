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
if [ "$cc" = "emcc" ]; then
    mkdir -p web

    cp index.html web/index.html

    find assets -type f -printf "--preload-file %p " | xargs \
        $cc $l -sUSE_SDL=1 -sUSE_SDL_IMAGE=1 -sLEGACY_GL_EMULATION \
        -sGL_FFP_ONLY -sGL_UNSAFE_OPTS=0 -sMODULARIZE=1 -sALLOW_MEMORY_GROWTH \
        --use-preload-plugins -o web/index.js
else
    $cc $l -o ld59 -lSDL -lSDL_image -lGL -lGLU -lm $ldflags
fi

if [ $? -ne 0 ]; then
    echo "-- Build failed!"
    exit 1
fi
