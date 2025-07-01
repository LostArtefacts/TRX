# start with a completely empty image
FROM scratch AS stage0

# set environment variables
ENV ARCH="x86_64"
ENV ARC_="amd64"
ENV PATH="/bin"
ENV JOBS=4

################################################################################
# STAGE 1: BOOTSTRAP THE CONTAINER IMAGE
################################################################################

#-------------------------------------------------------------------------------
# Toybox
#-------------------------------------------------------------------------------

FROM stage0 AS toybox

# copy the toybox executable and set its shell as default
# https://landley.net/toybox/downloads/binaries/0.8.12/toybox-x86_64
WORKDIR /usr/bin
COPY "toybox-${ARCH}" ./toybox
SHELL ["/usr/bin/toybox", "sh"]

# create symlinks for all toybox subcommands except shells
RUN -c  for f in `./toybox`; do ln -sf toybox "./${f}"; done
RUN -c  rm -f bash sh

# update the default shell command and create /bin
RUN -c  cp toybox sh
RUN -c  ln -s usr/bin /bin
SHELL ["sh", "-c"]

#-------------------------------------------------------------------------------
# Curl
#-------------------------------------------------------------------------------

FROM toybox AS curl

# copy the curl executable
# https://github.com/moparisthebest/static-curl/releases/download/v8.11.0/curl-amd64
WORKDIR /bin
COPY "curl-${ARC_}" ./curl

# download SSL certificates
WORKDIR /etc/ssl/certs
RUN curl -kLo ca-certificates.crt https://curl.se/ca/cacert.pem

# fallback nameserver
# RUN echo "nameserver 8.8.8.8" >> /etc/resolv.conf

# test connectivity
#RUN curl -I https://suckless.org

#-------------------------------------------------------------------------------
# XZ
#-------------------------------------------------------------------------------

FROM curl AS xz

# download the XZ decompressor
ENV CURFIL="xz.gz"
ENV CURURL="https://github.com/user-attachments/files/20087410/xz.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"

# extract
WORKDIR /bin
RUN gunzip "/tmp/xz.gz"
RUN mv /tmp/xz ./xz
RUN chmod a+x xz

#-------------------------------------------------------------------------------
# Zig
#-------------------------------------------------------------------------------

FROM xz AS zig

# download the zig compiler
ENV CURFIL="zig-linux-x86_64-0.12.0.tar.xz"
ENV CURURL="https://ziglang.org/download/0.12.0/zig-linux-x86_64-0.12.0.tar.xz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"

# extract
WORKDIR /opt
RUN tar -xaf "/tmp/zig-linux-${ARCH}-0.12.0.tar.xz"
RUN ln -s "zig-linux-${ARCH}-0.12.0" zig

# create compiler commands
WORKDIR /bin
RUN echo '#!/bin/sh' \
    | tee   ./zig           ./zig-ar        ./zig-objcopy   ./zig-ranlib  \
            ./zig-gnu-c++   ./zig-gnu-cc    ./zig-musl-c++  ./zig-musl-cc \
    > /dev/null
RUN echo 'zig ar "$@"'                                  >>  ./zig-ar
RUN echo 'zig c++ -target '${ARCH}'-linux-gnu.2.7 "$@"' >>  ./zig-gnu-c++
RUN echo 'zig cc  -target '${ARCH}'-linux-gnu.2.7 "$@"' >>  ./zig-gnu-cc
RUN echo 'zig c++ -target '${ARCH}'-linux-musl "$@"'    >>  ./zig-musl-c++
RUN echo 'zig cc  -target '${ARCH}'-linux-musl "$@"'    >>  ./zig-musl-cc
RUN echo 'zig objcopy "$@"'                             >>  ./zig-objcopy
RUN echo 'zig ranlib "$@"'                              >>  ./zig-ranlib
RUN echo 'export ZIG_GLOBAL_CACHE_DIR=/tmp/.zig-cache/global'           \
    'ZIG_LOCAL_CACHE_DIR=/tmp/.zig-cache/local && /opt/zig/zig "$@"'    \
                                                        >> ./zig
RUN chmod a+x ./zig*

#-------------------------------------------------------------------------------
# zld
#-------------------------------------------------------------------------------

FROM xz AS zld

# download the zld linker
ENV CURFIL="zld-0.0.4-x86_64-linux-musl.tar.xz"
ENV CURURL="https://github.com/kubkon/zld/releases/download/v0.0.4/zld-0.0.4-x86_64-linux-musl.tar.xz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"

# extract
WORKDIR /tmp
RUN tar -xaf "zld-0.0.4-${ARCH}-linux-musl.tar.xz"
RUN cp "zld-0.0.4-${ARCH}-linux-musl/bin/zld" /bin/ld.zld

#-------------------------------------------------------------------------------
# pdpmake
#-------------------------------------------------------------------------------

FROM zig AS pdpmake

# download pdpmake
ENV CURFIL="pdpmake-2.0.3.tgz"
ENV CURURL="https://frippery.org/make/pdpmake-2.0.3.tgz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"

# extract and compile
WORKDIR /tmp
RUN tar -xaf "pdpmake-2.0.3.tgz"
WORKDIR "pdpmake-2.0.3"
RUN zig-musl-cc -O2 -s -o /bin/pdpmake *.c

#-------------------------------------------------------------------------------
# Finalize stage
#-------------------------------------------------------------------------------

FROM stage0 AS stage1

# retrieve artifacts
COPY --from=toybox  /                   /
COPY --from=curl    /bin/curl           /bin/
COPY --from=curl    /etc/ssl/certs/     /etc/ssl/certs/
COPY --from=xz      /bin/xz             /bin/
COPY --from=zig     /bin/               /bin/
COPY --from=zig     /opt/               /opt/
COPY --from=zld     /bin/ld.zld         /bin/
COPY --from=pdpmake /bin/pdpmake        /bin/
COPY --from=pdpmake /tmp/.zig-cache     /tmp/.zig-cache

################################################################################
# STAGE 2: BUILD MISSING CORE UTILS
################################################################################

#-------------------------------------------------------------------------------
# oksh (required for autotools)
#-------------------------------------------------------------------------------

FROM stage1 AS oksh

# download oksh
ENV CURFIL="oksh-7.5.tar.gz"
ENV CURURL="https://github.com/ibara/oksh/releases/download/oksh-7.5/oksh-7.5.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"

# extract and compile
WORKDIR /tmp
RUN tar -xaf "oksh-7.5.tar.gz"
WORKDIR "oksh-7.5"
RUN true                                                >  ./pconfig.h
RUN echo "#ifndef __dead"                               >> ./pconfig.h
RUN echo "#define __dead __attribute__((__noreturn__))" >> ./pconfig.h
RUN echo "#endif"                                       >> ./pconfig.h
RUN echo "#define NO_CURSES"                            >> ./pconfig.h
RUN echo "#define HAVE_ST_MTIM"                         >> ./pconfig.h
RUN zig-musl-cc -O2 -DEMACS -DVI -D_GNU_SOURCE -s -o /bin/sh *.c

#-------------------------------------------------------------------------------
# sbase (expr, tr)
#-------------------------------------------------------------------------------

FROM stage1 AS sbase

# download sbase
ENV CURFIL="sbase-0.1.tar.gz"
ENV CURURL="https://dl.suckless.org/sbase/sbase-0.1.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"

# extract and compile
WORKDIR /tmp
RUN tar -xaf "sbase-0.1.tar.gz"
WORKDIR "sbase-0.1"
RUN pdpmake PREFIX=/usr CC="zig-musl-cc" AR="zig-ar" RANLIB="zig-ranlib" \
            expr tr
RUN cp expr tr /bin/

#-------------------------------------------------------------------------------
# wak (awk)
#-------------------------------------------------------------------------------

FROM stage1 AS wak

# download wak
ENV CURFIL="mono.c"
ENV CURURL="https://github.com/raygard/wak/raw/1a9e049/monosrc/mono.c"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"

# compile
WORKDIR /tmp
RUN sed -i -e 's/__DATE__/"Jan 01 1970"/' -e 's/__TIME__/"00:00:00"/' mono.c
RUN zig-musl-cc -O2 -std=c99 -D_POSIX_C_SOURCE=200809L -s -o /bin/awk mono.c

#-------------------------------------------------------------------------------
# bzip2
#-------------------------------------------------------------------------------

FROM stage1 AS bzip2

# download bzip2
ENV CURFIL="bzip2-1.0.8.tar.gz"
ENV CURURL="https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"

# extract and compile
WORKDIR /tmp
RUN tar -xaf "bzip2-1.0.8.tar.gz"
WORKDIR "bzip2-1.0.8"
RUN pdpmake PREFIX=/usr CC="zig-musl-cc" LDFLAGS="-s" \
            AR="zig-ar" RANLIB="zig-ranlib"

# copy to /bin
RUN cp bzip2 bzip2recover bzdiff bzgrep bzmore /bin/

#-------------------------------------------------------------------------------
# u-config (pkg-config)
#-------------------------------------------------------------------------------

FROM stage1 AS uconfig

# download uconfig
ENV CURFIL="u-config-0.33.3.tar.gz"
ENV CURURL="https://github.com/skeeto/u-config/releases/download/v0.33.3/u-config-0.33.3.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"

# extract and compile
WORKDIR /tmp
RUN tar -xaf "u-config-0.33.3.tar.gz"
WORKDIR "u-config-0.33.3"
RUN zig-musl-cc \
    -DPKG_CONFIG_LIBDIR='"/ext/lib/pkgconfig:/ext/share/pkgconfig"' \
    -DPKG_CONFIG_SYSTEM_INCLUDE_PATH='""' \
    -DPKG_CONFIG_SYSTEM_LIBRARY_PATH='""' \
    -O2 -s -o /bin/pkg-config generic_main.c

#-------------------------------------------------------------------------------
# Finalize stage
#-------------------------------------------------------------------------------

FROM stage1 AS stage2

# retrieve artifacts
COPY --from=oksh    /bin/sh             /bin/
COPY --from=sbase   /bin/expr /bin/tr   /bin/
COPY --from=wak     /bin/awk            /bin/
COPY --from=bzip2   /bin/bzip2          /bin/
COPY --from=uconfig /bin/pkg-config     /bin/

################################################################################
# STAGE 3: BUILD BUILD TOOLS
################################################################################

#-------------------------------------------------------------------------------
# GNU Make
#-------------------------------------------------------------------------------

FROM stage2 AS gmake
ENV CC="zig-musl-cc"
ENV CPP="${CC} -E"
ENV CFLAGS="-O2"
ENV LD="ld.zld"
ENV LDFLAGS="-s"
ENV AR="zig-ar"
ENV RANLIB="zig-ranlib"
ENV MAKE="pdpmake"

# download gmake
ENV CURFIL="make-4.4.1.tar.gz"
ENV CURURL="https://ftpmirror.gnu.org/gnu/make/make-4.4.1.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"

# extract and compile
WORKDIR /tmp
RUN tar -xaf "make-4.4.1.tar.gz"
WORKDIR "make-4.4.1"
RUN ./configure --prefix=/usr --disable-nls --disable-rpath --enable-year2038
RUN pdpmake
RUN cp ./make /bin/make

#-------------------------------------------------------------------------------
# Python 3
#-------------------------------------------------------------------------------

FROM stage2 AS python
COPY --from=gmake /bin/make /bin/

# set build variables
ENV CC="zig-musl-cc"
ENV CPP="${CC} -E"
ENV LD="ld.zld"
ENV AR="zig-ar"
ENV RANLIB="zig-ranlib"
ENV OBJCOPY="zig-objcopy"
ENV MAKE="make"
ENV PKG_CONFIG="pkg-config --static"

# download zlib
ENV CURFIL="zlib-1.3.1.tar.gz"
ENV CURURL="https://zlib.net/zlib-1.3.1.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"

# extract and compile
WORKDIR /tmp
RUN tar -xaf "zlib-1.3.1.tar.gz"
WORKDIR "zlib-1.3.1"
RUN ./configure --prefix=/ext --static
RUN ${MAKE} -j ${JOBS} install

# download python
ENV CURFIL="Python-3.7.9.tar.xz"
ENV CURURL="https://www.python.org/ftp/python/3.7.9/Python-3.7.9.tar.xz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"

# download module list
ENV CURFIL="Modules3.txt"
ENV CURURL="https://pastebin.com/raw/VhcZk9yp"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN dos2unix "${CURFIL}"

# extract and configure
WORKDIR /tmp
RUN tar -xaf "Python-3.7.9.tar.xz"
WORKDIR "Python-3.7.9"
RUN sed -i -e 's|DATE, TIME|"Jan  1 1970", "00:00:00"|' ./Modules/getbuildinfo.c
RUN cat ./Modules/Setup.dist    >> ./Modules/Setup
RUN cat ../Modules3.txt         >> ./Modules/Setup

# compile
RUN CFLAGS="`${PKG_CONFIG} --cflags zlib`" \
    LDFLAGS="`${PKG_CONFIG} --libs zlib` -static -s" \
    ./configure --prefix="/opt/python3" --disable-shared \
    ax_cv_c_float_words_bigendian=no
RUN ${MAKE} -j ${JOBS} LINKFORSHARED=" " install

# create launcher
RUN true                                    >  /bin/python3
RUN echo '#!/bin/sh'                        >> /bin/python3
RUN echo 'export PYTHONHOME=/opt/python3'   >> /bin/python3
RUN echo '${PYTHONHOME}/bin/python3 "$@"'   >> /bin/python3
RUN chmod a+x /bin/python3

# test python
RUN python3 -V

#-------------------------------------------------------------------------------
# Meson
#-------------------------------------------------------------------------------

FROM stage2 AS meson
COPY --from=python /bin/python3     /bin/
COPY --from=python /opt/python3/    /opt/python3/

# download meson
ENV CURFIL="meson-1.3.0.tar.gz"
ENV CURURL="https://github.com/mesonbuild/meson/releases/download/1.3.0/meson-1.3.0.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"

# download meson patch
ENV CURFIL="zigld.ptc"
ENV CURURL="https://pastebin.com/raw/YXyk8PfB"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"

# extract and patch
WORKDIR /opt/meson
RUN tar -xaf "/tmp/meson-1.3.0.tar.gz" --strip-components 1
RUN patch -p0 < /tmp/zigld.ptc

# create launcher
RUN ln -sf /opt/meson/meson.py /bin/meson
RUN chmod a+x /bin/meson

# test meson
RUN meson --help

#-------------------------------------------------------------------------------
# Ninja
#-------------------------------------------------------------------------------

FROM stage2 AS ninja
COPY --from=python /bin/python3     /bin/
COPY --from=python /opt/python3/    /opt/python3/

# set build variables
ENV CC="zig-musl-cc"
ENV CPP="${CC} -E"
ENV CXX="zig-musl-c++"
ENV CXXCPP="${CXX} -E"
ENV LD="ld.zld"
ENV AR="zig-ar"
ENV RANLIB="zig-ranlib"
ENV OBJCOPY="zig-objcopy"
ENV MAKE="make"
ENV PKG_CONFIG="pkg-config --static"

# download ninja
ENV CURFIL="ninja-1.11.1.tar.gz"
ENV CURURL="https://github.com/ninja-build/ninja/archive/refs/tags/v1.11.1.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"

# extract and compile
WORKDIR /tmp
RUN tar -xaf "ninja-1.11.1.tar.gz"
WORKDIR "ninja-1.11.1"
RUN python3 ./configure.py --bootstrap

# create launcher
RUN cp ./ninja /bin/ninja
RUN ninja --version

#-------------------------------------------------------------------------------
# UPX (TODO)
#-------------------------------------------------------------------------------

#FROM stage2 AS upx

#-------------------------------------------------------------------------------
# Finalize stage
#-------------------------------------------------------------------------------

FROM stage2 AS stage3

# retrieve artifacts
COPY --from=gmake   /bin/make       /bin/
COPY --from=gmake   /tmp/.zig-cache /tmp/.zig-cache
COPY --from=python  /bin/python3    /bin/
COPY --from=python  /opt/python3/   /opt/python3/
COPY --from=meson   /opt/meson/     /opt/meson/
COPY --from=meson   /bin/meson      /bin/
COPY --from=ninja   /bin/ninja      /bin/

# set build variables
ENV CC="zig-gnu-cc"
ENV CPP="${CC} -E"
ENV LD="ld.zld"
ENV AR="zig-ar"
ENV RANLIB="zig-ranlib"
ENV OBJCOPY="zig-objcopy"
ENV MAKE="make"
ENV PKG_CONFIG="pkg-config --static"
ENV CPATH="/ext/include"

# set cross compilation variables
ENV HOST="x86_64-linux-gnu"
ENV BUILD="x86_64-beos"
ENV CC_FOR_BUILD="zig-musl-cc"

# create program directory
ENV APPDIR="/opt/TR1X"
RUN mkdir -p "${APPDIR}/licenses"

################################################################################
# STAGE 4: BUILD TR1X DEPENDENCIES
################################################################################

#-------------------------------------------------------------------------------
# tirpc (for shared X11 only)
#-------------------------------------------------------------------------------

FROM stage3 AS tirpc
ENV CPATH="${CPATH}:/ext/include/tirpc"

# install tirpc
ENV CURFIL="libtirpc-1.2.6.tar.bz2"
ENV CURURL="https://sourceforge.net/projects/libtirpc/files/libtirpc/1.2.6/libtirpc-1.2.6.tar.bz2/download"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "libtirpc-1.2.6.tar.bz2"
WORKDIR "libtirpc-1.2.6"
RUN ./configure --prefix=/ext --enable-shared --disable-static \
    --disable-symvers --disable-gssapi --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install
RUN cp ./COPYING "${APPDIR}/licenses/tirpc.txt"

#-------------------------------------------------------------------------------
# X + GL prototypes
#-------------------------------------------------------------------------------

FROM tirpc AS xproto
ENV CFLAGS=

# install xproto
ENV CURFIL="xproto-7.0.16.tar.gz"
ENV CURURL="https://www.x.org/releases/X11R7.5/src/proto/xproto-7.0.16.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "xproto-7.0.16.tar.gz"
WORKDIR "xproto-7.0.16"
RUN ./configure --prefix=/ext --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install

# install xextproto
ENV CURFIL="xextproto-7.1.1.tar.gz"
ENV CURURL="https://www.x.org/releases/X11R7.5/src/proto/xextproto-7.1.1.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "xextproto-7.1.1.tar.gz"
WORKDIR "xextproto-7.1.1"
RUN ./configure --prefix=/ext --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install

# install kbproto
ENV CURFIL="kbproto-1.0.4.tar.gz"
ENV CURURL="https://www.x.org/releases/X11R7.5/src/proto/kbproto-1.0.4.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "kbproto-1.0.4.tar.gz"
WORKDIR "kbproto-1.0.4"
RUN ./configure --prefix=/ext --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install

# install inputproto
ENV CURFIL="inputproto-2.0.tar.gz"
ENV CURURL="https://www.x.org/releases/X11R7.5/src/proto/inputproto-2.0.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "inputproto-2.0.tar.gz"
WORKDIR "inputproto-2.0"
RUN ./configure --prefix=/ext --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install

# install bigreqsproto
ENV CURFIL="bigreqsproto-1.1.0.tar.gz"
ENV CURURL="https://www.x.org/releases/X11R7.5/src/proto/bigreqsproto-1.1.0.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "bigreqsproto-1.1.0.tar.gz"
WORKDIR "bigreqsproto-1.1.0"
RUN ./configure --prefix=/ext --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install

# install xcmiscproto
ENV CURFIL="xcmiscproto-1.2.0.tar.gz"
ENV CURURL="https://www.x.org/releases/X11R7.5/src/proto/xcmiscproto-1.2.0.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "xcmiscproto-1.2.0.tar.gz"
WORKDIR "xcmiscproto-1.2.0"
RUN ./configure --prefix=/ext --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install

# install xf86bigfontproto (optional)
ENV CURFIL="xf86bigfontproto-1.2.0.tar.gz"
ENV CURURL="https://www.x.org/releases/X11R7.5/src/proto/xf86bigfontproto-1.2.0.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "xf86bigfontproto-1.2.0.tar.gz"
WORKDIR "xf86bigfontproto-1.2.0"
RUN ./configure --prefix=/ext --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install

# install xtrans
ENV CURFIL="xtrans-1.2.5.tar.gz"
ENV CURURL="https://www.x.org/releases/X11R7.5/src/lib/xtrans-1.2.5.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "xtrans-1.2.5.tar.gz"
WORKDIR "xtrans-1.2.5"
RUN ./configure --prefix=/ext --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install

# install glproto
ENV CURFIL="glproto-1.4.10.tar.gz"
ENV CURURL="https://www.x.org/releases/X11R7.5/src/proto/glproto-1.4.10.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "glproto-1.4.10.tar.gz"
WORKDIR "glproto-1.4.10"
RUN ./configure --prefix=/ext --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install

#-------------------------------------------------------------------------------
# Xau
#-------------------------------------------------------------------------------

FROM xproto AS xau
ENV CFLAGS="-fno-sanitize=undefined -fno-sanitize-trap=undefined"

# install libXau
ENV CURFIL="libXau-1.0.5.tar.gz"
ENV CURURL="https://www.x.org/releases/X11R7.5/src/lib/libXau-1.0.5.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "libXau-1.0.5.tar.gz"
WORKDIR "libXau-1.0.5"
RUN ./configure --prefix=/ext --disable-static --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install
RUN cp ./COPYING "${APPDIR}/licenses/libxau.txt"

#-------------------------------------------------------------------------------
# X11
#-------------------------------------------------------------------------------

FROM xau AS x11
ENV CFLAGS="-fno-sanitize=undefined -fno-sanitize-trap=undefined \
-Wno-incompatible-function-pointer-types"

# install libX11
ENV CURFIL="libX11-1.3.2.tar.gz"
ENV CURURL="https://www.x.org/releases/X11R7.5/src/lib/libX11-1.3.2.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "libX11-1.3.2.tar.gz"
WORKDIR "libX11-1.3.2"
# cpp fix
RUN sed -i.bak -e "s/< conftest.\$ac_ext/conftest.\$ac_ext/g" ./configure
RUN ./configure --prefix=/ext --disable-static --without-xcb --without-perl \
    --enable-malloc0returnsnull --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install
RUN cp ./COPYING "${APPDIR}/licenses/libx11.txt"

#-------------------------------------------------------------------------------
# Xext
#-------------------------------------------------------------------------------

FROM x11 AS xext
ENV CFLAGS="-fno-sanitize=undefined -fno-sanitize-trap=undefined"

# install libXext
ENV CURFIL="libXext-1.1.1.tar.gz"
ENV CURURL="https://www.x.org/releases/X11R7.5/src/lib/libXext-1.1.1.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "libXext-1.1.1.tar.gz"
WORKDIR "libXext-1.1.1"
# cpp fix
RUN sed -i.bak -e "s/< conftest.\$ac_ext/conftest.\$ac_ext/g" ./configure
RUN ./configure --prefix=/ext --disable-static \
    --enable-malloc0returnsnull --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install
RUN cp ./COPYING "${APPDIR}/licenses/libxext.txt"

#-------------------------------------------------------------------------------
# Xrender
#-------------------------------------------------------------------------------

FROM xext AS xrender
ENV CFLAGS="-fno-sanitize=undefined -fno-sanitize-trap=undefined"

# install renderproto
ENV CURFIL="renderproto-0.11.tar.gz"
ENV CURURL="https://www.x.org/releases/X11R7.5/src/proto/renderproto-0.11.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "renderproto-0.11.tar.gz"
WORKDIR "renderproto-0.11"
RUN ./configure --prefix=/ext --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install

# install libXrender
ENV CURFIL="libXrender-0.9.5.tar.gz"
ENV CURURL="https://www.x.org/releases/X11R7.5/src/lib/libXrender-0.9.5.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "libXrender-0.9.5.tar.gz"
WORKDIR "libXrender-0.9.5"
RUN ./configure --prefix=/ext --disable-static \
    --enable-malloc0returnsnull --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install
RUN cp ./COPYING "${APPDIR}/licenses/libxrender.txt"

#-------------------------------------------------------------------------------
# XRandR
#-------------------------------------------------------------------------------

FROM xrender AS xrandr
ENV CFLAGS="-fno-sanitize=undefined -fno-sanitize-trap=undefined"

# install randrproto
ENV CURFIL="randrproto-1.3.1.tar.gz"
ENV CURURL="https://www.x.org/releases/X11R7.5/src/proto/randrproto-1.3.1.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "randrproto-1.3.1.tar.gz"
WORKDIR "randrproto-1.3.1"
RUN ./configure --prefix=/ext --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install

# install libXrandr
ENV CURFIL="libXrandr-1.3.0.tar.gz"
ENV CURURL="https://www.x.org/releases/X11R7.5/src/lib/libXrandr-1.3.0.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "libXrandr-1.3.0.tar.gz"
WORKDIR "libXrandr-1.3.0"
RUN ./configure --prefix=/ext --disable-static \
    --enable-malloc0returnsnull --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install
RUN cp ./COPYING "${APPDIR}/licenses/libxrandr.txt"

#-------------------------------------------------------------------------------
# glvnd
#-------------------------------------------------------------------------------

FROM xrandr AS glvnd
ENV CFLAGS="-DX_GLXCreateContextAttribsARB=34"

# install libglvnd
ENV CURFIL="libglvnd-1.7.0.tar.gz"
ENV CURURL="https://gitlab.freedesktop.org/glvnd/libglvnd/uploads/c24806c283070dc70700234ca8ffacf8/libglvnd-1.7.0.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "libglvnd-1.7.0.tar.gz"
WORKDIR "libglvnd-1.7.0"
RUN ./configure --prefix=/ext --enable-glx --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install
RUN tail -n21 README.md > "${APPDIR}/licenses/libglvnd.txt"

#-------------------------------------------------------------------------------
# ALSA
#-------------------------------------------------------------------------------

FROM stage3 AS alsa

# install libasound
ENV CURFIL="alsa-lib-1.0.11.tar.bz2"
ENV CURURL="https://www.alsa-project.org/files/pub/lib/alsa-lib-1.0.11.tar.bz2"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "alsa-lib-1.0.11.tar.bz2"
WORKDIR "alsa-lib-1.0.11"
# remove rpath and unnecessary c++ requirement
RUN sed -i.bak  -e 's|.${wl}--rpath.*||g' -e "5291s/.*/}/" ./configure
RUN ./configure --prefix=/ext --disable-static --enable-shared \
    --disable-python --disable-python2 --without-versioned \
    --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install
RUN cp ./COPYING "${APPDIR}/licenses/alsa.txt"

#-------------------------------------------------------------------------------
# SDL2
#-------------------------------------------------------------------------------

FROM stage3 AS sdl2
COPY --from=glvnd   /ext/   /ext/
COPY --from=alsa    /ext/   /ext/

# install libSDL2
ENV CURFIL="SDL2-2.28.5.tar.gz"
ENV CURURL="https://github.com/libsdl-org/SDL/releases/download/release-2.28.5/SDL2-2.28.5.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "SDL2-2.28.5.tar.gz"
WORKDIR "SDL2-2.28.5"
RUN LDFLAGS="`${PKG_CONFIG} --libs x11 alsa`" \
    ./configure --prefix=/ext --build=${BUILD} --host=${HOST}               \
    --enable-static --disable-shared --disable-rpath                        \
    --disable-sensor --disable-power --disable-locale                       \
    --disable-dbus --disable-ime --disable-ibus --disable-fcitx             \
    --disable-video-wayland --disable-video-directfb --disable-video-vulkan \
    --enable-video-opengl --enable-video-opengles                           \
    --enable-video-x11 --enable-x11-shared --with-x                         \
    --enable-audio --disable-oss --enable-alsa --enable-alsa-shared
RUN ${MAKE} -j ${JOBS} install
RUN cp ./LICENSE.txt "${APPDIR}/licenses/sdl2.txt"

#-------------------------------------------------------------------------------
# zlib
#-------------------------------------------------------------------------------

FROM stage3 AS zlib

# install zlib
ENV CURFIL="zlib-1.3.1.tar.gz"
ENV CURURL="https://zlib.net/zlib-1.3.1.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "zlib-1.3.1.tar.gz"
WORKDIR "zlib-1.3.1"
RUN ./configure --prefix=/ext --static
RUN ${MAKE} -j ${JOBS} install
RUN cp ./LICENSE "${APPDIR}/licenses/zlib.txt"

#-------------------------------------------------------------------------------
# PCRE2
#-------------------------------------------------------------------------------

FROM stage3 AS pcre2

# install libpcre2
ENV CURFIL="pcre2-10.44.tar.gz"
ENV CURURL="https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.44/pcre2-10.44.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "pcre2-10.44.tar.gz"
WORKDIR "pcre2-10.44"
RUN ./configure --prefix=/ext --enable-static --disable-shared \
    --build=${BUILD} --host=${HOST}
RUN ${MAKE} -j ${JOBS} install
RUN cp ./LICENCE "${APPDIR}/licenses/pcre2.txt"

#-------------------------------------------------------------------------------
# FFmpeg
#-------------------------------------------------------------------------------

FROM stage3 AS ffmpeg

# install libav*
ENV CURFIL="FFmpeg-n4.4.1.tar.gz"
ENV CURURL="https://github.com/FFmpeg/FFmpeg/archive/refs/tags/n4.4.1.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "FFmpeg-n4.4.1.tar.gz"
WORKDIR "FFmpeg-n4.4.1"
# defining HAVE_SYSCTL=0 does not work so we need this
RUN sed -i.bak -e "s/HAVE_SYSCTL/0/g" ./libavutil/cpu.c
RUN ./configure --prefix=/ext --enable-static --disable-shared \
    --enable-cross-compile --enable-small --cc="${CC}" --cxx="${CXX}" \
    --ar="${AR}" --ranlib="${RANLIB}" --pkg-config="${PKG_CONFIG}" \
    --disable-postproc --disable-avfilter --disable-programs \
    --arch=x86 --disable-asm --disable-debug --disable-doc \
    --disable-network --disable-htmlpages --disable-manpages \
    --disable-podpages --disable-txtpages
RUN ${MAKE} -j ${JOBS} install
RUN cp ./LICENSE.md "${APPDIR}/licenses/ffmpeg.txt"

#-------------------------------------------------------------------------------
# uthash
#-------------------------------------------------------------------------------

FROM stage3 AS uthash

# install uthash
ENV CURFIL="uthash-2.3.0.tar.gz"
ENV CURURL="https://github.com/troydhanson/uthash/archive/v2.3.0.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "uthash-2.3.0.tar.gz"
WORKDIR "uthash-2.3.0"
RUN mkdir -p /ext/include/
RUN cp ./src/*.h /ext/include/
RUN cp ./LICENSE "${APPDIR}/licenses/uthash.txt"

#-------------------------------------------------------------------------------
# Finalize stage
#-------------------------------------------------------------------------------

FROM stage3 AS stage4

# retrieve artifacts
COPY --from=sdl2    /ext/   /ext/
COPY --from=zlib    /ext/   /ext/
COPY --from=pcre2   /ext/   /ext/
COPY --from=ffmpeg  /ext/   /ext/
COPY --from=uthash  /ext/   /ext/

################################################################################
# STAGE 5: BUILD TR1X
################################################################################

FROM stage4 AS stage5
ENV PYTHONPATH="/opt/meson"

# extract sources
ENV CURFIL="TRX-tr1-4.7.1.tar.gz"
ENV CURURL="https://github.com/LostArtefacts/TRX/archive/refs/tags/tr1-4.7.1.tar.gz"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN tar -xaf "TRX-tr1-4.7.1.tar.gz"
ENV TOP="/tmp/TRX-tr1-4.7.1"

# create native and cross build file
WORKDIR /tmp
RUN printf "%s\n%s\n%s\n%s\n" \
    "[binaries]" \
    "c = 'zig-musl-cc'" \
    "c_ld = 'ld.zld'" \
    "ar = 'zig-ar'" \
    > build.txt
RUN printf "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n" \
    "[binaries]" \
    "c = 'zig-gnu-cc'" \
    "c_ld = 'ld.zld'" \
    "ar = 'zig-ar'" \
    "" \
    "[host_machine]" \
    "system = 'linux'" \
    "cpu_family = 'x86_64'" \
    "cpu = 'x86_64'" \
    "endian = 'little'" \
    > cross.txt

# create dummy git command
RUN printf "%s\n%s\n" '#!/bin/sh' 'exit 1' > /bin/git
RUN chmod a+x /bin/git

# patch generate_init for Python 3.7 (original requires 3.10)
ENV CURFIL="generate_init"
ENV CURURL="https://pastebin.com/raw/cWdJQrip"
WORKDIR /tmp
#COPY "${CURFIL}" ./
RUN [ -f "${CURFIL}" ] || curl -Lo "${CURFIL}" "${CURURL}"
RUN cp ./generate_init ${TOP}/tools/tr1/
RUN chmod a+x ${TOP}/tools/tr1/generate_init

# compile tr1
WORKDIR "${TOP}/src/tr1"
RUN LDFLAGS="-s `${PKG_CONFIG} --libs zlib`" \
    meson setup --optimization 2 \
    --native-file /tmp/build.txt \
    --cross-file  /tmp/cross.txt \
    --prefix /ext build
RUN sed -i -e "s|-Wl,-rpath-link,/ext/lib||" ./build/build.ninja
RUN meson compile -C build

# package game
WORKDIR build
RUN tar -cvjf "/TR1X-4.7.1.tbz" *
