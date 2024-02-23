# TR1X building toolchain.
#
# This is a multi-stage Docker image. It is designed to keep the final image
# size low. Each stage builds an external dependency. The final stage takes the
# artifacts (binaries, includes etc.) from previous stages and installs all the
# tools necessary to build TR1X.

# MinGW
FROM ubuntu:latest as mingw

# don't prompt during potential installation/update of tzinfo
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Warsaw

RUN apt-get update \
    && apt-get upgrade -y \
    && apt-get install -y \
        gcc-mingw-w64-i686 \
        g++-mingw-w64-i686 \
        git \
        make



# zlib
FROM mingw as zlib
RUN git clone https://github.com/madler/zlib
RUN cd zlib \
    && make -f win32/Makefile.gcc \
        SHARED_MODE=1 \
        BINARY_PATH=/ext/bin \
        INCLUDE_PATH=/ext/include \
        LIBRARY_PATH=/ext/lib \
        PREFIX=i686-w64-mingw32- \
        -j 4 install


# libav
FROM mingw as libav
RUN apt-get install -y \
    nasm
RUN git clone \
    --depth 1 \
    --branch "n4.4.1" \
    https://github.com/FFmpeg/FFmpeg
COPY --from=zlib /ext/ /usr/i686-w64-mingw32/
RUN cd FFmpeg \
    && ./configure \
        --arch=x86 \
        --target-os=mingw32 \
        --cross-prefix=i686-w64-mingw32- \
        --prefix=/ext/ \
        --cc=i686-w64-mingw32-gcc \
        --strip=i686-w64-mingw32-strip \
        --pkg-config=i686-w64-mingw32-pkg-config \
        --enable-gpl \
        --enable-decoder=pcx \
        --enable-decoder=png \
        --enable-decoder=gif \
        --enable-decoder=mjpeg \
        --enable-decoder=mpeg4 \
        --enable-decoder=mdec \
        --enable-decoder=h264 \
        --enable-decoder=h264_qsv \
        --enable-decoder=libopenh264 \
        --enable-decoder=png \
        --enable-demuxer=mov \
        --enable-demuxer=avi \
        --enable-demuxer=h264 \
        --enable-demuxer=str \
        --enable-demuxer=image2 \
        --enable-zlib \
        --enable-static \
        --enable-small \
        --disable-debug \
        --disable-ffplay \
        --disable-ffprobe \
        --disable-doc \
        --disable-network \
        --disable-htmlpages \
        --disable-manpages \
        --disable-podpages \
        --disable-txtpages \
        --disable-asm \
    && make -j 4 \
    && make install



# SDL
FROM mingw as sdl
RUN git clone https://github.com/libsdl-org/SDL -b SDL2
RUN apt-get install -y automake
RUN cd SDL \
    && aclocal -I acinclude \
    && autoconf \
    && mkdir build \
    && cd build \
    && ../configure \
        --host=i686-w64-mingw32 \
        --build=i686-pc-mingw32 \
        --prefix=/ext/ \
        --enable-shared \
        --enable-static \
    && make -j 4 \
    && make install

# SDL3 - Compiles fine, but meson cannot find it.
# TODO - fix that once SDL reaches maturity and releases 3.x as a stable version
# RUN git clone https://github.com/libsdl-org/SDL
# RUN apt-get install -y cmake
# RUN cd SDL \
#     && mkdir build \
#     && cd build \
#     && cmake .. \
#         --toolchain build-scripts/cmake-toolchain-mingw64-i686.cmake \
#         -DCMAKE_INSTALL_PREFIX=/ext/  \
#         -DCMAKE_BUILD_TYPE=Release \
#         -DSDL_VULKAN=ON \
#         -DSDL_TEST=ON \
#     && cmake --build . \
#     && make -j 4 \
#     && make install



# TR1X
FROM mingw

# set the build dir - actual files are mounted with a Docker volume
RUN mkdir /app
WORKDIR /app

COPY --from=zlib /ext/ /ext/
COPY --from=libav /ext/ /ext/
COPY --from=sdl /ext/ /ext/

# system dependencies
# configure pkgconfig manually
# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=967969
ENV PKG_CONFIG_LIBDIR=/ext/lib/
ENV PKG_CONFIG_PATH=/ext/lib/pkgconfig/
RUN apt-get install -y \
        mingw-w64-tools \
        pkg-config \
        upx \
        python3-pip \
    && python3 -m pip install \
        pyjson5 \
        meson \
        ninja

ENTRYPOINT ["/app/docker/game/entrypoint.sh"]
