CWD = $(shell pwd)
HOST_USER_UID = $(shell id -u)
HOST_USER_GID = $(shell id -g)

debug: docker
	mkdir -p build
	docker run --rm \
		--user $(HOST_USER_UID):$(HOST_USER_GID) \
		-e TARGET=debug \
		-v $(CWD):/app/ \
		tomb1main

release:
	mkdir -p build
	docker run --rm \
		--user $(HOST_USER_UID):$(HOST_USER_GID) \
		-e TARGET=release \
		-v $(CWD):/app/ \
		tomb1main

docker:
	docker build -t tomb1main . -f docker/Dockerfile

clean:
	-find build/ -type f -delete
	-find build/ -mindepth 1 -empty -type d -delete

lint:
	clang-format -i **/*.c **/*.h

test_base:
	cp build/*.exe test/
	cp build/*.dll test/
	ln -rsft test/ bin/*
	rm -f test/Winplay.dll

test: build test_base
	WINEARCH=win32 MESA_GL_VERSION_OVERRIDE=3.3 wine test/Tomb1Main.exe

test_gold: build test_base
	WINEARCH=win32 MESA_GL_VERSION_OVERRIDE=3.3 wine test/Tomb1Main.exe -gold

.PHONY: debug release docker clean lint test_base test test_gold
