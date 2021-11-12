FROM ubuntu:latest

RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y gcc-mingw-w64-i686 make git python3-pip && \
    python3 -m pip install pyjson5 meson ninja

RUN mkdir /app
ADD . /app
WORKDIR /app

RUN chmod 0666 /app/src/init.c

CMD ["sh", "-c", "make debug_linux >/dev/null; make build"]
