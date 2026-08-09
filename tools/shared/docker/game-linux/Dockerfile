# TRX building toolchain for Linux.
#
# This is a multi-stage Docker image. It is designed to keep the final image
# size low. Each stage builds an external dependency. The final stage takes the
# artifacts (binaries, includes etc.) from previous stages and installs all the
# tools necessary to build TRX.

FROM debian:bookworm AS base

# enable backports
RUN echo "deb http://deb.debian.org/debian bookworm-backports main" > /etc/apt/sources.list.d/backports.list

# LLVM repo for clang-20
RUN apt-get update && apt-get install -y --no-install-recommends wget gnupg ca-certificates && \
    wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc > /dev/null && \
    echo "deb http://apt.llvm.org/bookworm/ llvm-toolchain-bookworm main" >> /etc/apt/sources.list.d/llvm.list && \
    echo "deb http://apt.llvm.org/bookworm/ llvm-toolchain-bookworm-20 main" >> /etc/apt/sources.list.d/llvm.list && \
    apt-get update

# don't prompt during potential installation/update of tzinfo
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Warsaw

RUN apt-get update \
    && apt-get upgrade -y \
    && apt-get install -y \
        git \
        make \
        clang-20 \
        lld-20


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



# librubberband
FROM base AS rubberband
RUN apt-get install -y \
    gcc \
    g++ \
    pkg-config \
    python3-pip \
    && python3 -m pip install --break-system-packages \
        meson \
        ninja
RUN git clone https://github.com/breakfastquay/rubberband --branch v4.0.0
RUN cd rubberband \
    && meson setup build \
        --prefix=/ext/ \
        --libdir=lib \
        --default-library=static \
        -Dfft=builtin \
        -Dresampler=builtin \
        -Djni=disabled \
        -Dladspa=disabled \
        -Dlv2=disabled \
        -Dvamp=disabled \
        -Dcmdline=disabled \
        -Dtests=disabled \
    && ninja -C build \
    && ninja -C build install
# the generated file names neither the C++ runtime nor libm, which whoever
# links the static library has to be told about
RUN sed -i '/^Libs.private:/d' /ext/lib/pkgconfig/rubberband.pc \
    && echo 'Libs.private: -lstdc++ -lm' >> /ext/lib/pkgconfig/rubberband.pc



# libav
FROM base AS libav
RUN apt-get install -y \
    nasm \
    gcc \
    g++ \
    pkg-config \
    zlib1g-dev
COPY --from=rubberband /ext/ /ext/
ENV PKG_CONFIG_PATH=/ext/lib/pkgconfig/
RUN git clone \
    https://github.com/FFmpeg/FFmpeg
RUN cd FFmpeg && git checkout 066432ebcf
COPY ./tools/ffmpeg_flags.txt /tmp/ffmpeg_flags.txt
RUN cd FFmpeg \
    && ./configure \
        --arch=x86 \
        --prefix=/ext/ \
        --enable-static \
        --disable-shared \
        --pkg-config-flags=--static \
        --extra-cflags=-I/ext/include \
        --extra-ldflags=-L/ext/lib \
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
    libasound2-dev \
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



# GLEW
FROM base AS glew
RUN git clone https://github.com/nigels-com/glew.git
RUN apt-get install -y \
    build-essential \
    libxmu-dev \
    libxi-dev \
    libgl-dev \
    python3
RUN cd glew/auto \
    && PYTHON=python3 make
RUN mkdir -p /ext/lib \
    && export \
        GLEW_NO_GLU=-DGLEW_NO_GLU \
        GLEW_DEST=/ext \
    && cd glew \
    && make \
    && make install
RUN sed -i "s/Cflags: .*/\\0 -DGLEW_STATIC /" /ext/lib/pkgconfig/glew.pc

## Lua
FROM base AS lua
RUN apt-get update && apt-get install -y wget build-essential libreadline-dev
RUN wget https://www.lua.org/ftp/lua-5.4.8.tar.gz \
    && tar xzf lua-5.4.8.tar.gz \
    && cd lua-5.4.8 \
    && make linux \
    && make INSTALL_TOP=/ext install

# Provide pkg-config file for Lua
RUN mkdir -p /ext/lib/pkgconfig
COPY tools/shared/docker/lua.pc /ext/lib/pkgconfig/lua.pc





# TRX
FROM base

# set the build dir - actual files are mounted with a Docker volume
RUN mkdir /app
WORKDIR /app

# package dependencies
RUN apt-get install -y \
    zlib1g-dev \
    libgl1-mesa-dev \
    g++

# tooling dependencies
# configure pkgconfig manually
# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=967969
ENV PKG_CONFIG_LIBDIR=/ext/lib/
ENV PKG_CONFIG_PATH=/ext/lib/pkgconfig/
RUN apt-get install -y \
        pkg-config \
        git \
        ccache \
        python3-pip \
    && python3 -m pip install --break-system-packages \
        pyjson5 \
        meson \
        ninja
# Regular dependencies
RUN apt-get install -y \
    upx-ucl/bookworm-backports \
    uthash-dev

## manually built dependencies
COPY --from=lua /ext/ /ext/
COPY --from=libav /ext/ /ext/
COPY --from=sdl /ext/ /ext/
COPY --from=backtrace /ext/ /ext/
COPY --from=pcre2 /ext/ /ext/
COPY --from=glew /ext/ /ext/

ENV PYTHONPATH=/app/tools/
ENV CCACHE_BASEDIR=/app/
ENV CCACHE_COMPILERCHECK=content
ENV CCACHE_DIR=/app/.cache/ccache/linux
ENV CC="clang-20"
ENV CXX="clang++-20"
ENTRYPOINT ["/app/tools/shared/docker/game-linux/entrypoint.sh"]
