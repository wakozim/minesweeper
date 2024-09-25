#!/bin/sh

set -xe

CFLAGS="-O3 -std=c99 -Wall -Wextra -pedantic -ggdb -I."
LIBS="-lm -lglfw -ldl -lpthread -lGL -lrt -lX11"

mkdir -p ./build/

clang $CFLAGS -o ./build/minesweeper main.c -L./raylib/raylib-5.0_linux_amd64/lib/ -l:libraylib.a -no-pie -D_DEFAULT_SOURCE $LIBS

x86_64-w64-mingw32-gcc -DPLATFORM_DESKTOP -mwindows -Wall -Wextra -ggdb -I./raylib/raylib-5.0_win64_mingw-w64/include/ $CFLAGS -o ./build/minesweeper.exe main.c -L./raylib/raylib-5.0_win64_mingw-w64/lib -l:libraylib.a -lwinmm -lgdi32 -static
