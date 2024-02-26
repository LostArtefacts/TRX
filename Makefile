CWD = $(shell pwd)
HOST_USER_UID = $(shell id -u)
HOST_USER_GID = $(shell id -g)

define build
	$(eval TARGET := $(1))
	mkdir -p build
	docker run --rm \
		--user $(HOST_USER_UID):$(HOST_USER_GID) \
		-e TARGET="$(TARGET)" \
		-v $(CWD):/app/ \
		rrdash/tr1x:latest
endef

define build-linux
	$(eval TARGET := $(1))
	mkdir -p build
	docker run --rm \
		--user $(HOST_USER_UID):$(HOST_USER_GID) \
		-e TARGET="$(TARGET)" \
		-v $(CWD):/app/ \
		rrdash/tr1x-linux:latest
endef

debug:
	$(call build,debug)

debugopt:
	$(call build,debugoptimized)

release:
	$(call build,release)

build-docker-image:
	docker build --progress plain . -f docker/game-win/Dockerfile -t rrdash/tr1x

build-docker-image-linux:
	docker build --progress plain . -f docker/game-linux/Dockerfile -t rrdash/tr1x-linux

debug-linux:
	$(call build-linux,debug)

debugopt-linux:
	$(call build-linux,debugoptimized)

release-linux:
	$(call build-linux,release)

clean:
	-find build/ -type f -delete
	-find tools/ -type f \( -ipath '*/out/*' -or -ipath '*/bin/*' -or -ipath '*/obj/*' \) -delete
	-find . -mindepth 1 -empty -type d -delete

lint_imports:
	tools/sort_imports

lint_format:
	bash -c 'shopt -s globstar; clang-format -i **/*.c **/*.h'

lint: lint_format lint_imports

installer:
	docker build . -f docker/installer/Dockerfile -t rrdash/tr1x_installer
	docker run --rm \
		--user $(HOST_USER_UID):$(HOST_USER_GID) \
		--network host \
		-v $(CWD):/app/ \
		rrdash/tr1x_installer

config:
	docker build . -f docker/config/Dockerfile -t rrdash/tr1x_config
	docker run --rm \
		--user $(HOST_USER_UID):$(HOST_USER_GID) \
		--network host \
		-v $(CWD):/app/ \
		rrdash/tr1x_config

.PHONY: debug debugopt release clean lint_imports lint_format lint installer config
