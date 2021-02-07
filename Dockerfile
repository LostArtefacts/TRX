FROM ubuntu:latest

RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y gcc-mingw-w64-i686

WORKDIR /
ADD src /src

ENTRYPOINT ["/entrypoint.sh"]
