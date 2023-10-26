#!/bin/sh

set -xe

CFLAGS="-O3 -Wall -Wextra -ggdb -I./thirdparty/ -I. `pkg-config --cflags raylib`"
LIBS="-lm `pkg-config --libs raylib` -lglfw -ldl -lpthread"

mkdir -p ./build/
clang $CFLAGS -o ./build/minesweeper main.c $LIBS
