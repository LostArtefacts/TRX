#!/bin/sh
set -x
set -e

EXE_FILE=TR1X

if [ ! -f /app/build/linux/build.ninja ]; then
    meson --buildtype "$TARGET" /app/build/linux/ --pkg-config-path=$PKG_CONFIG_PATH
fi

cd /app/build/linux
meson compile

if [ "$TARGET" = release ]; then
    if ! upx -t "$EXE_FILE"; then
        strip "$EXE_FILE"
        upx "$EXE_FILE"
    fi
fi
