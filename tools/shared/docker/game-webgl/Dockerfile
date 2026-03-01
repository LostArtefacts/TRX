# TRX building toolchain for WebGL/Emscripten.
#
# Unlike the Linux/Windows images which build every dependency from source,
# the WebGL image is lightweight: most deps come from Emscripten ports
# (SDL2, zlib, Lua) and the only vendored library (libwebpdecoder.a) is
# pre-built and checked into the repository.

FROM emscripten/emsdk:latest

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Warsaw

# Meson, Ninja, and Python packages needed by the build scripts
RUN pip install --break-system-packages \
        pyjson5 \
        meson \
        ninja \
        Pillow

RUN mkdir -p /app
WORKDIR /app

ENV PYTHONPATH=/app/tools/
ENTRYPOINT ["/app/tools/shared/docker/game-webgl/entrypoint.sh"]
