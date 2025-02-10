# TR2X building toolchain for Linux.
#
# This is a multi-stage Docker image. It is designed to keep the final image
# size low. Each stage builds an external dependency. The final stage takes the
# artifacts (binaries, includes etc.) from previous stages and installs all the
# tools necessary to build TR2X.

FROM ubuntu:latest AS base

# don't prompt during potential installation/update of tzinfo
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Warsaw

RUN apt-get update \
    && apt-get upgrade -y \
    && apt-get install -y \
        git \
        make



# pcre2
FROM base AS pcre2
RUN apt-get install -y git gcc autoconf libtool
RUN git clone https://github.com/PCRE2Project/pcre2
RUN cd pcre2 \
    && autoreconf -fi \
    && ./configure \
        --prefix=/ext/ \
    && make -j 4 \
    && make install



# libbacktrace
FROM base AS backtrace
RUN apt-get install -y git gcc autoconf libtool
RUN git clone https://github.com/LostArtefacts/libbacktrace/
RUN cd libbacktrace \
    && autoreconf -fi \
    && ./configure \
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
    --branch "n7.1" \
    https://github.com/FFmpeg/FFmpeg
COPY ./tools/ffmpeg_flags.txt /tmp/ffmpeg_flags.txt
RUN cd FFmpeg \
    && ./configure \
        --arch=x86 \
        --prefix=/ext/ \
        --enable-static \
        --disable-shared \
        $(cat /tmp/ffmpeg_flags.txt) \
    && make -j 4 \
    && make install



# SDL
FROM base AS sdl
RUN git clone https://github.com/libsdl-org/SDL -b SDL2
RUN apt-get install -y \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libpulse-dev \
    libxkbcommon-dev \
    libwayland-client0 \
    libwayland-cursor0 \
    libwayland-dev \
    libwayland-egl1 \
    automake \
    gcc \
    libxrandr-dev \
    libxext-dev
RUN cd SDL \
    && aclocal -I acinclude \
    && autoconf \
    && mkdir sdl_build \
    && cd sdl_build \
    && ../configure \
        --prefix=/ext/ \
        --enable-video-x11-xrandr \
        --enable-shared \
        --enable-static \
    && make -j 4 \
    && make install



# TR2X
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
    && python3 -m pip install --break-system-packages \
        pyjson5 \
        meson \
        ninja
# Regular dependencies
RUN apt-get install -y \
        upx \
        uthash-dev

# manually built dependencies
COPY --from=libav /ext/ /ext/
COPY --from=sdl /ext/ /ext/
COPY --from=backtrace /ext/ /ext/
COPY --from=pcre2 /ext/ /ext/

ENV PYTHONPATH=/app/tools/
ENTRYPOINT ["/app/tools/tr2/docker/game-linux/entrypoint.sh"]
