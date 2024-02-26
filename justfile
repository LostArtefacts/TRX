CWD := `pwd`
HOST_USER_UID := `id -u`
HOST_USER_GID := `id -g`

default: (build-win "debug")

_docker_build dockerfile tag force="0":
    #!/usr/bin/env sh
    docker images | grep {{tag}} >/dev/null
    if [ $? -eq 0 ] && [ "{{force}}" = "0" ]; then
        echo "Docker image {{dockerfile}} is already built"
    else
        echo "Building Docker image: {{dockerfile}} → {{tag}}"
        docker build \
            --progress plain \
            . \
            -f {{dockerfile}} \
            -t {{tag}}
    fi

_docker_run *args:
    @echo "Running docker image: {{args}}"
    docker run \
        --rm \
        --user \
        {{HOST_USER_UID}}:{{HOST_USER_GID}} \
        --network host \
        -v {{CWD}}:/app/ \
        {{args}}


image-win force="1":       (_docker_build "docker/game-win/Dockerfile" "rrdash/tr1x" force)
image-linux force="1":     (_docker_build "docker/game-linux/Dockerfile" "rrdash/tr1x-linux" force)
image-config force="1":    (_docker_build "docker/config/Dockerfile" "rrdash/tr1x-config" force)
image-installer force="1": (_docker_build "docker/installer/Dockerfile" "rrdash/tr1x-installer" force)

build-win target='debug':   (image-win "0")       (_docker_run "-e" "TARGET="+target "rrdash/tr1x")
build-linux target='debug': (image-linux "0")     (_docker_run "-e" "TARGET="+target "rrdash/tr1x-linux")
build-config:               (image-config "0")    (_docker_run "rrdash/tr1x-config")
build-installer:            (image-installer "0") (_docker_run "rrdash/tr1x-installer")

clean:
    -find build/ -type f -delete
    -find tools/ -type f \( -ipath '*/out/*' -or -ipath '*/bin/*' -or -ipath '*/obj/*' \) -delete
    -find . -mindepth 1 -empty -type d -delete

lint-imports:
    tools/sort_imports

lint-format:
    #!/usr/bin/env bash
    shopt -s globstar
    clang-format -i **/*.{c,h}

lint: (lint-imports) (lint-format)
