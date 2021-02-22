CC=i686-w64-mingw32-gcc
CFLAGS=-Wall -Isrc \
	-DTOMB1M_FEAT_CHEATS \
	-DTOMB1M_FEAT_EXTENDED_MEMORY \
	-DTOMB1M_FEAT_UI \
	-DTOMB1M_FEAT_GAMEPLAY \
	-DTOMB1M_FEAT_LEVEL_FIXES \
	-DTOMB1M_FEAT_NOCD

C_FILES = $(shell find src/ -type f -name '*.c')
O_FILES = $(patsubst src/%.c, build/%.o, $(C_FILES))

build: all
	$(CC) $(CFLAGS) $(shell find build -type f -iname '*.o') -static -ldbghelp -shared -o build/Tomb1Main.dll

all: $(O_FILES)

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
