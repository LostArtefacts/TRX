FROM ubuntu:latest

# system dependencies
RUN apt-get update \
    && apt-get upgrade -y \
    && apt-get install -y \
        gcc-mingw-w64-i686 \
        g++-mingw-w64-i686 \
        make \
        git \
        upx \
        python3-pip \
        nasm \
    && python3 -m pip install \
        pyjson5 \
        meson \
        ninja

# cross compile third party dependencies
RUN mkdir /ext
WORKDIR /ext

# zlib
RUN git clone https://github.com/madler/zlib
RUN cd zlib \
    && make -f win32/Makefile.gcc \
        SHARED_MODE=1 \
        BINARY_PATH=/usr/i686-w64-mingw32/bin \
        INCLUDE_PATH=/usr/i686-w64-mingw32/include \
        LIBRARY_PATH=/usr/i686-w64-mingw32/lib \
        PREFIX=i686-w64-mingw32- \
        -j 4 install

# ffmpeg
RUN git clone https://github.com/FFmpeg/FFmpeg --depth 1
RUN cd FFmpeg \
    && ./configure \
        --arch=x86 \
        --target-os=mingw32 \
        --cross-prefix=i686-w64-mingw32- \
        --prefix=/usr/i686-w64-mingw32 \
        --cc=i686-w64-mingw32-gcc \
        --pkg-config=i686-w64-mingw32-pkg-config \
        --enable-zlib \
        --enable-shared \
        --enable-static \
        --disable-ffplay \
        --disable-ffprobe \
        --disable-doc \
        --disable-network \
        --disable-htmlpages \
        --disable-manpages \
        --disable-podpages \
        --disable-txtpages \
    && make -j 4 \
    && make install

# set the build dir - actual files are mounted with a Docker volume
RUN mkdir /app
WORKDIR /app

ENTRYPOINT ["/app/docker/entrypoint.sh"]
