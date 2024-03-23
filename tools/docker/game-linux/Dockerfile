# TR1X building toolchain for Linux.
#
# This is a multi-stage Docker image. It is designed to keep the final image
# size low. Each stage builds an external dependency. The final stage takes the
# artifacts (binaries, includes etc.) from previous stages and installs all the
# tools necessary to build TR1X.

FROM ubuntu:latest as base

# don't prompt during potential installation/update of tzinfo
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Warsaw

RUN apt-get update \
    && apt-get upgrade -y \
    && apt-get install -y \
        git \
        make



# libbacktrace
FROM base AS backtrace
RUN apt-get install -y git gcc autoconf libtool
RUN git clone https://github.com/LostArtefacts/libbacktrace/
RUN cd libbacktrace && \
    autoreconf -fi && \
    ./configure \
        --prefix=/ext/ \
    && make -j 4 \
    && make install



# libav
FROM base AS libav
RUN apt-get install -y \
    nasm \
    gcc \
    zlib1g-dev
RUN git clone \
    --depth 1 \
    --branch "n4.4.1" \
    https://github.com/FFmpeg/FFmpeg
RUN cd FFmpeg \
    && ./configure \
        --arch=x86 \
        --prefix=/ext/ \
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
FROM base AS sdl
RUN git clone https://github.com/libsdl-org/SDL -b SDL2
RUN apt-get install -y \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libpulse-dev \
    automake \
    gcc \
    libxext-dev
RUN cd SDL \
    && aclocal -I acinclude \
    && autoconf \
    && mkdir sdl_build \
    && cd sdl_build \
    && ../configure \
        --prefix=/ext/ \
        --enable-shared \
        --enable-static \
    && make -j 4 \
    && make install



# UPX
FROM base AS upx

RUN mkdir /ext/
WORKDIR /ext/

RUN apt-get install -y wget xz-utils
RUN wget https://github.com/upx/upx/releases/download/v4.1.0/upx-4.1.0-amd64_linux.tar.xz
RUN tar -xvf upx-*.tar.xz
RUN ln -s upx-*/upx



# TR1X
FROM base

# set the build dir - actual files are mounted with a Docker volume
RUN mkdir /app
WORKDIR /app

# package dependencies
RUN apt-get install -y \
    zlib1g-dev \
    libgl1-mesa-dev

# tooling dependencies
# configure pkgconfig manually
# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=967969
ENV PKG_CONFIG_LIBDIR=/ext/lib/
ENV PKG_CONFIG_PATH=/ext/lib/pkgconfig/
RUN apt-get install -y \
        pkg-config \
        git \
        python3-pip \
    && python3 -m pip install \
        pyjson5 \
        meson \
        ninja

# manually built dependencies
COPY --from=upx /ext/upx /usr/local/bin/upx
COPY --from=libav /ext/ /ext/
COPY --from=sdl /ext/ /ext/
COPY --from=backtrace /ext/ /ext/

ENTRYPOINT ["/app/tools/docker/game-linux/entrypoint.sh"]
