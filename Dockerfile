FROM ubuntu:latest

RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y gcc-mingw-w64-i686 make git

RUN mkdir /app
WORKDIR /app
ADD .git .git
ADD src src
ADD test test
ADD Makefile Makefile

CMD ["make", "build"]
