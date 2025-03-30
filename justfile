CWD := `pwd`
HOST_USER_UID := `id -u`
HOST_USER_GID := `id -g`

default: (tr1-build-win "debug") (tr2-build-win "debug")

_docker_push tag:
    docker push {{tag}}

_docker_build dockerfile tag force="0":
    #!/usr/bin/env sh
    if [ "{{force}}" = "0" ]; then
        docker images --format '{''{.Repository}}' | grep '^{{tag}}$' >/dev/null
        if [ $? -eq 0 ]; then
            echo "Docker image {{tag}} found"
            exit 0
        fi
        echo "Docker image {{tag}} not found, trying to download from DockerHub"
        if docker pull {{tag}}; then
            echo "Docker image {{tag}} downloaded from DockerHub"
            exit 0
        fi
        echo "Docker image {{tag}} not found, trying to build"
    fi

    echo "Building Docker image: {{dockerfile}} → {{tag}}"
    docker build \
        . \
        -f {{dockerfile}} \
        -t {{tag}}

_docker_run *args:
    @echo "Running docker image: {{args}}"
    docker run \
        --rm \
        --user \
        {{HOST_USER_UID}}:{{HOST_USER_GID}} \
        -v {{CWD}}:/app/ \
        {{args}}


tr1-image-linux force="1":         (_docker_build "tools/tr1/docker/game-linux/Dockerfile" "rrdash/tr1x-linux" force)
tr1-image-win force="1":           (_docker_build "tools/tr1/docker/game-win/Dockerfile" "rrdash/tr1x" force)
tr1-image-win-config force="1":    (_docker_build "tools/tr1/docker/config/Dockerfile" "rrdash/tr1x-config" force)
tr1-image-win-installer force="1": (_docker_build "tools/tr1/docker/installer/Dockerfile" "rrdash/tr1x-installer" force)

tr1-push-image-linux:              (tr1-image-linux "0") (_docker_push "rrdash/tr1x-linux")
tr1-push-image-win:                (tr1-image-win "0") (_docker_push "rrdash/tr1x")

tr1-build-linux target='debug':    (tr1-image-linux "0")         (_docker_run "-e" "TARGET="+target "rrdash/tr1x-linux")
tr1-build-win target='debug':      (tr1-image-win "0")           (_docker_run "-e" "TARGET="+target "rrdash/tr1x")
tr1-build-win-config:              (tr1-image-win-config "0")    (_docker_run "rrdash/tr1x-config")
tr1-build-win-installer:           (tr1-image-win-installer "0") (_docker_run "rrdash/tr1x-installer")

tr1-package-linux target='release': (tr1-build-linux target) (_docker_run "rrdash/tr1x-linux" "package")
tr1-package-win target='release': (tr1-build-win target) (_docker_run "rrdash/tr1x" "package")
tr1-package-win-all target='release': (tr1-build-win target) (tr1-build-win-config) (_docker_run "rrdash/tr1x" "package")
tr1-package-win-installer target='release': (tr1-build-win target) (tr1-build-win-config) (_docker_run "rrdash/tr1x" "package" "-o" "tools/installer/TR1X_Installer/Resources/release.zip") (tr1-build-win-installer)
    #!/bin/sh
    git checkout "tools/installer/TR1X_Installer/Resources/release.zip"
    exe_name=TR1X-$(tools/get_version 1)-Installer.exe
    cp tools/installer/out/TR1X_Installer.exe "${exe_name}"
    echo "Created ${exe_name}"

tr2-image-linux force="1":            (_docker_build "tools/tr2/docker/game-linux/Dockerfile" "rrdash/tr2x-linux" force)
tr2-image-win force="1":              (_docker_build "tools/tr2/docker/game-win/Dockerfile" "rrdash/tr2x" force)
tr2-image-win-config force="1":       (_docker_build "tools/tr2/docker/config/Dockerfile" "rrdash/tr2x-config" force)
tr2-image-win-installer force="1":    (_docker_build "tools/tr2/docker/installer/Dockerfile" "rrdash/tr2x-installer" force)

tr2-push-image-linux:                 (tr2-image-linux "0") (_docker_push "rrdash/tr2x-linux")
tr2-push-image-win:                   (tr2-image-win "0") (_docker_push "rrdash/tr2x")

tr2-build-linux target='debug':       (tr2-image-linux "0")      (_docker_run "-e" "TARGET="+target "rrdash/tr2x-linux")
tr2-build-win target='debug':         (tr2-image-win "0")        (_docker_run "-e" "TARGET="+target "rrdash/tr2x")
tr2-build-win-config:                 (tr2-image-win-config "0") (_docker_run "rrdash/tr2x-config")
tr2-build-win-installer:              (tr2-image-win-installer "0") (_docker_run "rrdash/tr2x-installer")

tr2-package-linux target='release':   (tr2-build-linux target) (_docker_run "rrdash/tr2x-linux" "package")
tr2-package-win target='release':     (tr2-build-win target) (_docker_run "rrdash/tr2x" "package")
tr2-package-win-all target='release': (tr2-build-win target) (tr2-build-win-config) (_docker_run "rrdash/tr2x" "package")
tr2-package-win-installer target='release': (tr2-build-win target) (tr2-build-win-config) (_docker_run "rrdash/tr2x" "package" "-o" "tools/installer/TR2X_Installer/Resources/release.zip") (tr2-build-win-installer)
    #!/bin/sh
    git checkout "tools/installer/TR2X_Installer/Resources/release.zip"
    exe_name=TR2X-$(tools/get_version 1)-Installer.exe
    cp tools/installer/out/TR2X_Installer.exe "${exe_name}"
    echo "Created ${exe_name}"

output-release-name tr_version:
    tools/output_release_name {{tr_version}}

output-current-version tr_version:
    tools/get_version {{tr_version}}

output-current-changelog tr_version:
    tools/output_current_changelog {{tr_version}}

clean:
    -find build/ -type f -delete
    -find tools/ -type f \( -ipath '*/out/*' -or -ipath '*/bin/*' -or -ipath '*/obj/*' \) -delete
    -find . -mindepth 1 -empty -type d -delete

lint-imports:
    tools/sort_imports

lint-format:
    pre-commit run -a

lint: (lint-imports) (lint-format)
