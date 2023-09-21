#!/bin/sh
set -x
set -e

export CFLAGS=-DDOCKER_BUILD
EXE_FILE=Tomb1Main

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
