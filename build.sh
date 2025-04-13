#!/usr/bin/bash
cd "$(dirname $0)"
set -e

BUILD_DIR=web
SRC_DIR=src
mkdir -p "$BUILD_DIR"

emcc -Wall -Wextra -Wconversion -Werror -std=gnu23 "$SRC_DIR"/*.c -o "$BUILD_DIR/as.js" \
    -s EXPORTED_FUNCTIONS='["_assemble","_get_error_buf"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","UTF8ToString","stringToUTF8"]'

cp -l "$SRC_DIR"/*.{js,css,html} "$BUILD_DIR"
