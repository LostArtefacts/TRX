#!/bin/sh
set -x
set -e

export CFLAGS=-DDOCKER_BUILD

if [ ! -f /app/build/linux/build.ninja ]; then
    meson --buildtype "$TARGET" /app/build/linux/ --pkg-config-path=$PKG_CONFIG_PATH
fi

cd /app/build/linux; meson compile

if [ "$TARGET" = release ]; then
    exe_file=Tomb1Main
    if ! upx -t "$exe_file"; then
        strip "$exe_file" && upx "$exe_file"
    fi
fi
