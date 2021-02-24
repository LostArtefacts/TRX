CC=i686-w64-mingw32-gcc
WINDRES=i686-w64-mingw32-windres
CFLAGS=-Wall -Isrc \
	-DT1M_FEAT_CHEATS \
	-DT1M_FEAT_EXTENDED_MEMORY \
	-DT1M_FEAT_UI \
	-DT1M_FEAT_GAMEPLAY \
	-DT1M_FEAT_LEVEL_FIXES \
	-DT1M_FEAT_NOCD

VERSION = $(shell git describe --abbrev=7 --tags master)
C_FILES = $(shell find src/ -type f -name '*.c')
O_FILES = $(patsubst src/%.c, build/%.o, $(C_FILES))

build: all version
	$(CC) $(CFLAGS) $(shell find build -type f -iname '*.o') build/version.res -static -ldbghelp -shared -o build/Tomb1Main.dll

all: $(O_FILES)

version:
	sed s/{version}/$(VERSION)/g src/version.rc >build/version.rc
	$(WINDRES) build/version.rc -O coff -o build/version.res

clean:
	find build -type f -iname '*.o' -delete
	find build -type f -iname '*.dll' -delete
	find . -type d -empty -delete

build/%.o: src/%.c
	@mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -c "$<" -o "$@"

docker_build:
	docker build -t tr1main .
	docker run -v $(shell pwd)/build:/build tr1main

lint:
	clang-format-10 -i $(shell find src -type f -iname '*.c' -or -iname '*.h')

docs:
	docs/render_progress

.PHONY: clean build docker_build lint docs
