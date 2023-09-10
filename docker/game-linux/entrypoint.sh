#!/bin/sh
set -x
set -e

export CFLAGS=-DDOCKER_BUILD

if [ ! -f /app/build/linux/build.ninja ]; then
    meson --buildtype "$TARGET" /app/build/linux/ --pkg-config-path=$PKG_CONFIG_PATH
fi

cd /app/build/linux; meson compile

if [ "$TARGET" = release ]; then
    for file in Tomb1Main; do
        upx -t "$file" || ( strip "$file" && upx "$file" )
    done
fi

