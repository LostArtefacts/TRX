CC=i686-w64-mingw32-gcc
WINDRES=i686-w64-mingw32-windres
LDFLAGS=-ldbghelp
CFLAGS=-Wall -Isrc \
	-DT1M_FEAT_HAIR \
	-DT1M_FEAT_CHEATS \
	-DT1M_FEAT_EXTENDED_MEMORY \
	-DT1M_FEAT_UI \
	-DT1M_FEAT_INPUT \
	-DT1M_FEAT_GAMEPLAY \
	-DT1M_FEAT_OG_FIXES \
	-DT1M_FEAT_NOCD

VERSION = $(shell git describe --abbrev=7 --tags master)
CWD = $(shell pwd)
C_FILES = $(shell find src/ -type f -name '*.c')
H_FILES = $(shell find src/ -type f -name '*.h')
O_FILES = $(patsubst src/%.c, build/src/%.o, $(C_FILES))
TEST_C_FILES = $(shell find test/ -type f -name '*.c')
TEST_H_FILES = $(shell find test/ -type f -name '*.h')
TEST_O_FILES = $(patsubst test/%.c, build/test/%.o, $(TEST_C_FILES))

build: $(O_FILES) version
	$(CC) $(CFLAGS) $(O_FILES) build/version.res $(LDFLAGS) -static -shared -o build/Tomb1Main.dll

debug: CFLAGS += -DDEBUG -g
debug: build

test: $(O_FILES) $(TEST_O_FILES)
	$(CC) $(CFLAGS) $(O_FILES) $(TEST_O_FILES) $(LDFLAGS) -o build/test.o
	chmod +x build/test.o
	build/test.o

version:
	sed s/{version}/$(VERSION)/g src/version.rc >build/version.rc
	$(WINDRES) build/version.rc -O coff -o build/version.res

clean:
	find build -type f -iname '*.o' -delete
	find build -type f -iname '*.dll' -delete
	rm -f build/version.res build/version.rc
	find . -type d -empty -delete

docker_build:
	docker build -t tomb1main .
	docker run --rm -v $(CWD)/.git:/app/.git -v $(CWD)/build:/app/build tomb1main

lint:
	clang-format-10 -i $(C_FILES) $(H_FILES) $(TEST_C_FILES) $(TEST_H_FILES)

docs:
	docs/render_progress

build/src/%.o: src/%.c
	@mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -c "$<" -o "$@"

build/test/%.o: test/%.c
	@mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -c "$<" -o "$@"

.PHONY: clean build docker_build lint docs test
