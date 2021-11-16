CWD = $(shell pwd)
HOST_USER_UID = $(shell id -u)
HOST_USER_GID = $(shell id -g)

build:
	cd build; meson compile

debug:
	meson --buildtype debug build/
release:
	meson --buildtype release build/
debug_linux:
	meson --buildtype debug build/ --cross meson_linux_mingw32.txt
release_linux:
	meson --buildtype release build/ --cross meson_linux_mingw32.txt

clean:
	-find build/ -type f -delete
	-find build/ -mindepth 1 -empty -type d -delete

lint:
	clang-format -i **/*.c **/*.h

docker:
	docker build -t tomb1main .

docker_build: docker
	-mkdir -p build
	docker run --rm \
		--user $(HOST_USER_UID):$(HOST_USER_GID) \
		-v $(CWD)/.git:/app/.git \
		-v $(CWD)/build:/app/build \
		-v $(CWD)/src/init.c:/app/src/init.c \
		tomb1main

.PHONY: release debug build clean lint docker docker_build
