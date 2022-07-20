#!/bin/sh
set -x
set -e

if [ ! -f /app/build/build.ninja ]; then
    meson --buildtype "$TARGET" /app/build/ --cross /app/docker/game/meson_linux_mingw32.txt --pkg-config-path=$PKG_CONFIG_PATH
fi

cd /app/build; meson compile

if [ "$TARGET" = release ]; then
    for file in *.exe; do
        upx -t "$file" || ( i686-w64-mingw32-strip "$file" && upx "$file" )
    done
fi
