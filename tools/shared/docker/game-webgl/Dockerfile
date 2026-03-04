# TRX building toolchain for WebGL/Emscripten.
#
# Most deps come from Emscripten ports (SDL2, zlib).  FFmpeg and Lua are
# cross-compiled to WASM in separate build stages so that the cost is
# paid once at image-creation time, not on every project build.

FROM emscripten/emsdk:latest AS base

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Warsaw

# Meson, Ninja, and Python packages needed by the build scripts
RUN pip install --break-system-packages \
        pyjson5 \
        meson \
        ninja \
        Pillow


# FFmpeg — cross-compiled to WASM static libraries
FROM base AS libav

RUN apt-get update && apt-get install -y wget xz-utils

RUN git clone \
    --depth 1 \
    --branch "n7.1" \
    https://github.com/FFmpeg/FFmpeg /tmp/FFmpeg

COPY ./tools/ffmpeg_flags_webgl.txt /tmp/ffmpeg_flags_webgl.txt

# Install into the Emscripten sysroot so that pkg-config finds the
# libraries automatically during the project build.
RUN cd /tmp/FFmpeg \
    && . /emsdk/emsdk_env.sh \
    && emconfigure ./configure \
        --prefix=/emsdk/upstream/emscripten/cache/sysroot/usr \
        --enable-cross-compile \
        --target-os=none \
        --arch=wasm32 \
        --cc=emcc \
        --ar=emar \
        --ranlib=emranlib \
        --enable-static \
        --disable-shared \
        --disable-pthreads \
        --disable-runtime-cpudetect \
        --disable-stripping \
        --extra-cflags="-pthread -sUSE_ZLIB=1" \
        --extra-ldflags="-pthread" \
        $(cat /tmp/ffmpeg_flags_webgl.txt) \
    && emmake make -j$(nproc) \
    && emmake make install \
    && rm -rf /tmp/FFmpeg /tmp/ffmpeg_flags_webgl.txt


# Lua — cross-compiled to WASM static library with -pthread.
# Built from source instead of using --use-port=contrib.lua because the
# Emscripten port system does not reliably propagate -pthread to port
# compilation, causing wasm-ld to reject the objects at link time.
FROM base AS lua

RUN apt-get update && apt-get install -y wget
RUN wget -qO /tmp/lua.tar.gz https://www.lua.org/ftp/lua-5.5.0.tar.gz \
    && mkdir /tmp/lua && tar xzf /tmp/lua.tar.gz -C /tmp/lua --strip-components=1

RUN cd /tmp/lua/src \
    && . /emsdk/emsdk_env.sh \
    && emcc -pthread -O2 -DLUA_COMPAT_5_4 -c \
        lapi.c lauxlib.c lbaselib.c lcode.c lcorolib.c lctype.c \
        ldblib.c ldebug.c ldo.c ldump.c lfunc.c lgc.c linit.c \
        liolib.c llex.c lmathlib.c lmem.c loadlib.c lobject.c \
        lopcodes.c loslib.c lparser.c lstate.c lstring.c lstrlib.c \
        ltable.c ltablib.c ltm.c lundump.c lutf8lib.c lvm.c lzio.c \
    && emar rcs liblua.a *.o \
    && mkdir -p /emsdk/upstream/emscripten/cache/sysroot/usr/lib/pkgconfig \
              /emsdk/upstream/emscripten/cache/sysroot/usr/include \
    && cp liblua.a /emsdk/upstream/emscripten/cache/sysroot/usr/lib/ \
    && cp /tmp/lua/src/lua.h /tmp/lua/src/luaconf.h /tmp/lua/src/lualib.h \
          /tmp/lua/src/lauxlib.h \
          /emsdk/upstream/emscripten/cache/sysroot/usr/include/ \
    && rm -rf /tmp/lua /tmp/lua.tar.gz

# Provide pkg-config file for Lua
COPY tools/shared/docker/game-webgl/lua.pc /emsdk/upstream/emscripten/cache/sysroot/usr/lib/pkgconfig/lua.pc


# Final image
FROM base

# pkg-config is needed for Meson to find FFmpeg libraries
RUN apt-get update && apt-get install -y pkg-config

# Carry over the FFmpeg static libraries and headers from the build stage
COPY --from=libav \
    /emsdk/upstream/emscripten/cache/sysroot/usr/ \
    /emsdk/upstream/emscripten/cache/sysroot/usr/

# Carry over the Lua static library and headers from the build stage
COPY --from=lua \
    /emsdk/upstream/emscripten/cache/sysroot/usr/ \
    /emsdk/upstream/emscripten/cache/sysroot/usr/

# Point pkg-config at the Emscripten sysroot so it finds FFmpeg's .pc files
ENV PKG_CONFIG_LIBDIR=/emsdk/upstream/emscripten/cache/sysroot/usr/lib/pkgconfig

RUN mkdir -p /app
WORKDIR /app

ENV PYTHONPATH=/app/tools/
ENTRYPOINT ["/app/tools/shared/docker/game-webgl/entrypoint.sh"]
