# TRX building toolchain for WebGL/Emscripten.
#
# Most deps come from Emscripten ports (SDL2, zlib, Lua).  FFmpeg is
# cross-compiled to WASM in a separate build stage so that the cost is
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


# Final image
FROM base

# Carry over the FFmpeg static libraries and headers from the build stage
COPY --from=libav \
    /emsdk/upstream/emscripten/cache/sysroot/usr/ \
    /emsdk/upstream/emscripten/cache/sysroot/usr/

RUN mkdir -p /app
WORKDIR /app

ENV PYTHONPATH=/app/tools/
ENTRYPOINT ["/app/tools/shared/docker/game-webgl/entrypoint.sh"]
