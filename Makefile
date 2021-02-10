CC=i686-w64-mingw32-gcc
CFLAGS=-W -static -shared

build: $(shell find src)
	$(CC) $(CFLAGS) $(shell find src -type f -iname '*.c') -ldbghelp -o build/TR1Main.dll

docker_build:
	docker build -t tr1main .
	docker run -v $(shell pwd)/build:/build tr1main

lint:
	clang-format-10 -i $(wildcard **/*.h **/*.c)

.PHONY: build docker_build lint
