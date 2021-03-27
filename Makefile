CC=i686-w64-mingw32-gcc
PYTHON=python3
WINDRES=i686-w64-mingw32-windres
LDFLAGS=-ldbghelp -lwinmm -ldsound -lddraw
CFLAGS=-Wall -Isrc

VERSION = $(shell git describe --abbrev=7 --tags master)
CWD = $(shell pwd)
C_FILES = $(shell find src/ -type f -name '*.c') build/init.c
H_FILES = $(shell find src/ -type f -name '*.h')
O_FILES = $(patsubst src/%.c, build/src/%.o, $(C_FILES))
TEST_C_FILES = $(shell find test/ -type f -name '*.c')
TEST_H_FILES = $(shell find test/ -type f -name '*.h')
TEST_O_FILES = $(patsubst test/%.c, build/test/%.o, $(TEST_C_FILES))
HOST_USER_UID = $(shell id -u)
HOST_USER_GID = $(shell id -g)

# building
build: $(O_FILES) version
	$(CC) $(CFLAGS) $(O_FILES) build/version.res $(LDFLAGS) -static -shared -o build/Tomb1Main.dll

debug: CFLAGS += -DDEBUG -g
debug: build

test: $(O_FILES) $(TEST_O_FILES)
	$(CC) $(CFLAGS) $(O_FILES) $(TEST_O_FILES) $(LDFLAGS) -o build/test.exe
	chmod +x build/test.exe
	build/test.exe

# physical files
build/src/%.o: src/%.c
	@mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -c "$<" -o "$@"

build/test/%.o: test/%.c
	@mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -c "$<" -o "$@"

build/init.c: Tomb1Main_gameflow.json5 src/generate_init.py
	$(PYTHON) src/generate_init.py

src/generate_init.py:
Tomb1Main_gameflow.json5:
	@:

version:
	sed s/{version}/$(VERSION)/g src/version.rc >build/version.rc
	$(WINDRES) build/version.rc -O coff -o build/version.res

# misc tasks
clean:
	find build -type f -iname '*.o' -delete
	find build -type f -iname '*.dll' -delete
	rm -f build/version.res build/version.rc build/init.c
	find . -type d -empty -delete

lint: $(C_FILES)
	clang-format-10 -i $(C_FILES) $(H_FILES) $(TEST_C_FILES) $(TEST_H_FILES)

docs:
	docs/render_progress

# docker builds
docker:
	docker build -t tomb1main .

docker_build: docker
	docker run --rm \
		--user $(HOST_USER_UID):$(HOST_USER_GID) \
		-v $(CWD)/.git:/app/.git \
		-v $(CWD)/build:/app/build \
		tomb1main

.PHONY: clean build docker docker_build lint docs test
