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
    && python3 -m pip install \
        pyjson5 \
        meson \
        ninja

# set the build dir - actual files are mounted with a Docker volume
RUN mkdir /app
WORKDIR /app

ENTRYPOINT ["/app/docker/entrypoint.sh"]
