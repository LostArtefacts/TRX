CWD = $(shell pwd)
HOST_USER_UID = $(shell id -u)
HOST_USER_GID = $(shell id -g)

define build
	$(eval TARGET := $(1))
	mkdir -p build
	docker run --rm \
		--user $(HOST_USER_UID):$(HOST_USER_GID) \
		--entrypoint /app/docker/game-win/entrypoint.sh \
		-e TARGET="$(TARGET)" \
		-v $(CWD):/app/ \
		rrdash/tomb1main:latest
endef

define build-linux
	$(eval TARGET := $(1))
	mkdir -p build
	docker run --rm \
		--user $(HOST_USER_UID):$(HOST_USER_GID) \
		--entrypoint /app/docker/game-linux/entrypoint.sh \
		-e TARGET="$(TARGET)" \
		-v $(CWD):/app/ \
		rrdash/tomb1main-linux:latest
endef

debug:
	$(call build,debug)

debugopt:
	$(call build,debugoptimized)

release:
	$(call build,release)

debug-linux:
	$(call build-linux,debug)

debugopt-linux:
	$(call build-linux,debugoptimized)

release-linux:
	$(call build-linux,release)

clean:
	-find build/ -type f -delete
	-find build/ -mindepth 1 -empty -type d -delete

imports:
	tools/sort_imports

lint:
	bash -c 'shopt -s globstar; clang-format -i **/*.c **/*.h'

installer:
	docker build . -f docker/installer/Dockerfile -t rrdash/tomb1main_installer
	docker run --rm \
		--user $(HOST_USER_UID):$(HOST_USER_GID) \
		--network host \
		-v $(CWD):/app/ \
		rrdash/tomb1main_installer

config:
	docker build . -f docker/config/Dockerfile -t rrdash/tomb1main_config
	docker run --rm \
		--user $(HOST_USER_UID):$(HOST_USER_GID) \
		--network host \
		-v $(CWD):/app/ \
		rrdash/tomb1main_config

.PHONY: debug debugopt release clean imports lint installer config
