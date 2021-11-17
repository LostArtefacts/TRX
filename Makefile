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

.PHONY: debug release docker clean lint
