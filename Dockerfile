FROM ubuntu:latest

RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y gcc-mingw-w64-i686 make

WORKDIR /
ADD src /src
ADD Makefile /Makefile

CMD ["make", "build"]
