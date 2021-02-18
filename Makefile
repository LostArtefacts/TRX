CC=i686-w64-mingw32-gcc
CFLAGS=-W -static -shared -Isrc

build: $(shell find src)
	$(CC) $(CFLAGS) $(shell find src -type f -iname '*.c') -ldbghelp -o build/Tomb1Main.dll

docker_build:
	docker build -t tr1main .
	docker run -v $(shell pwd)/build:/build tr1main

lint:
	clang-format-10 -i $(shell find src -type f -iname '*.c' -or -iname '*.h')

docs:
	docs/render_progress

.PHONY: build docker_build lint docs
