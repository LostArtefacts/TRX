#!/bin/sh
set -x
set -e

export CFLAGS=-DDOCKER_BUILD

if [ ! -f /app/build/win/build.ninja ]; then
    meson --buildtype "$TARGET" /app/build/win/ --cross /app/docker/game-win/meson_linux_mingw32.txt --pkg-config-path=$PKG_CONFIG_PATH
fi

cd /app/build/win; meson compile

if [ "$TARGET" = release ]; then
    for file in *.exe; do
        upx -t "$file" || ( i686-w64-mingw32-strip "$file" && upx "$file" )
    done
fi
