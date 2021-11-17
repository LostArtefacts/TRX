#!/bin/sh
if [ ! -d /app/build/meson-info ]; then
    if [ "$TARGET" = debug ]; then
        meson --buildtype debug /app/build/ --cross /app/docker/meson_linux_mingw32.txt
    elif [ "$TARGET" = release ]; then
        meson --buildtype release /app/build/ --cross /app/docker/meson_linux_mingw32.txt
    fi
fi

cd /app/build; meson compile
