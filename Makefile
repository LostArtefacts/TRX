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
		rrdash/tomb1main:latest
endef

debug:
	$(call build,debug)

debugopt:
	$(call build,debugoptimized)

release:
	$(call build,release)

clean:
	-find build/ -type f -delete
	-find build/ -mindepth 1 -empty -type d -delete

imports:
	scripts/sort_imports

lint:
	bash -c 'shopt -s globstar; clang-format -i **/*.c **/*.h'

test_base:
	cp build/*.exe test/

test_bin:
	rsync -r bin/ test/

test: build test_base
	WINEARCH=win32 MESA_GL_VERSION_OVERRIDE=3.3 wine test/Tomb1Main.exe

test_gold: build test_base
	WINEARCH=win32 MESA_GL_VERSION_OVERRIDE=3.3 wine test/Tomb1Main.exe -gold

.PHONY: debug debugopt release clean imports lint test_base test_bin test test_gold
