# TR2X building toolchain.
#
# This is a multi-stage Docker image. It is designed to keep the final image
# size low. Each stage builds an external dependency. The final stage takes the
# artifacts (binaries, includes etc.) from previous stages and installs all the
# tools necessary to build TR2X.

# MinGW
FROM ubuntu:latest AS mingw

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



# pcre
FROM mingw AS pcre2
RUN git clone https://github.com/PCRE2Project/pcre2
RUN apt-get -y install libtool
RUN cd pcre2 \
    && autoreconf -fi \
    && ./configure \
        --host=i686-w64-mingw32 \
        --prefix=/ext/ \
    && make -j 4 \
    && make install



# zlib
FROM mingw AS zlib
RUN git clone https://github.com/madler/zlib --branch=v1.3.1
RUN cd zlib \
    && make -f win32/Makefile.gcc \
        SHARED_MODE=1 \
        BINARY_PATH=/ext/bin \
        INCLUDE_PATH=/ext/include \
        LIBRARY_PATH=/ext/lib \
        PREFIX=i686-w64-mingw32- \
        -j 4 install



# libav
FROM mingw AS libav
RUN apt-get install -y \
    nasm
RUN git clone \
    --depth 1 \
    --branch "n7.1" \
    https://github.com/FFmpeg/FFmpeg
COPY --from=zlib /ext/ /usr/i686-w64-mingw32/
COPY ./tools/ffmpeg_flags.txt /tmp/ffmpeg_flags.txt
RUN cd FFmpeg \
    && ./configure \
        --arch=x86 \
        --target-os=mingw32 \
        --cross-prefix=i686-w64-mingw32- \
        --prefix=/ext/ \
        --cc=i686-w64-mingw32-gcc \
        --cxx=i686-w64-mingw32-g++ \
        --host-cc=i686-w64-mingw32-gcc \
        --strip=i686-w64-mingw32-strip \
        --pkg-config=i686-w64-mingw32-pkg-config \
        --enable-static \
        --disable-shared \
        $(cat /tmp/ffmpeg_flags.txt) \
    && make -j 4 \
    && make install



# SDL
FROM mingw AS sdl
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



# uthash
FROM mingw AS uthash

RUN mkdir /ext/
WORKDIR /tmp/

RUN apt-get install -y wget xz-utils
RUN wget https://github.com/troydhanson/uthash/archive/v2.3.0.tar.gz
RUN tar -xvf v2.3.0.tar.gz
RUN cp -rL uthash-2.3.0/* /ext/



# TR2X
FROM mingw

# set the build dir - actual files are mounted with a Docker volume
RUN mkdir /app
WORKDIR /app

COPY --from=pcre2 /ext/ /ext/
COPY --from=zlib /ext/ /ext/
COPY --from=libav /ext/ /ext/
COPY --from=sdl /ext/ /ext/
COPY --from=uthash /ext/ /ext/

# system dependencies
# configure pkgconfig manually
# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=967969
RUN apt-get install -y \
        mingw-w64-tools \
        pkg-config \
        upx \
        python3-pip \
    && python3 -m pip install --break-system-packages \
        pyjson5 \
        meson \
        ninja

ENV PKG_CONFIG_LIBDIR=/ext/lib/
ENV PKG_CONFIG_PATH=/ext/lib/pkgconfig/
ENV C_INCLUDE_PATH=/ext/include/
ENV PYTHONPATH=/app/tools/
ENTRYPOINT ["/app/tools/tr2/docker/game-win/entrypoint.sh"]
