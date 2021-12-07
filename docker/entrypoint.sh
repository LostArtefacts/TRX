#!/bin/sh
set -x
set -e

if [ ! -f /app/build/build.ninja ]; then
    meson --buildtype "$TARGET" /app/build/ --cross /app/docker/meson_linux_mingw32.txt --pkg-config-path=$PKG_CONFIG_PATH
fi

cp /usr/lib/gcc/i686-w64-mingw32/9.3-posix/libgcc_s_sjlj-1.dll /app/build/
cp /usr/i686-w64-mingw32/lib/libwinpthread-1.dll /app/build/
cp /ext/bin/swscale-5.dll /app/build/
cp /ext/bin/swresample-3.dll /app/build/
cp /ext/bin/avutil-56.dll /app/build/
cp /ext/bin/avfilter-7.dll /app/build/
cp /ext/bin/avcodec-58.dll /app/build/
cp /ext/bin/avdevice-58.dll /app/build/
cp /ext/bin/zlib1.dll /app/build/
cp /ext/bin/avformat-58.dll /app/build/

cd /app/build; meson compile

if [ "$TARGET" = release ]; then
    for file in *.dll *.exe; do
        upx -t "$file" || ( i686-w64-mingw32-strip "$file" && upx "$file" )
    done
fi
