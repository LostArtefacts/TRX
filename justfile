CWD := `pwd`
HOST_USER_UID := `id -u`
HOST_USER_GID := `id -g`
DOCKER_IMAGE_VERSION := "20251017.rev1"

default: (tr1-build-win "debug") (tr2-build-win "debug")

_docker_push tag:
    docker push {{tag}}:{{DOCKER_IMAGE_VERSION}}

_docker_build dockerfile tag force="0":
    #!/usr/bin/env sh
    full_tag="{{tag}}:{{DOCKER_IMAGE_VERSION}}"
    if [ "{{force}}" = "0" ]; then
        docker images --format '{''{.Repository}}:{''{.Tag}}' | grep '^'"$full_tag"'$' >/dev/null
        if [ $? -eq 0 ]; then
            echo "Docker image $full_tag found"
            exit 0
        fi
        echo "Docker image $full_tag not found, trying to download from DockerHub"
        if docker pull $full_tag; then
            echo "Docker image $full_tag downloaded from DockerHub"
            exit 0
        fi
        echo "Docker image $full_tag not found, trying to build"
    fi

    echo "Building Docker image: {{dockerfile}} → $full_tag"
    docker build \
        . \
        -f {{dockerfile}} \
        -t $full_tag

_docker_run tag *args:
    #!/usr/bin/env sh
    full_tag="{{tag}}:{{DOCKER_IMAGE_VERSION}}"
    echo "Running docker image: $full_tag {{args}}"
    docker run \
        --rm \
        --user \
        {{HOST_USER_UID}}:{{HOST_USER_GID}} \
        -v {{CWD}}:/app/ \
        $full_tag \
        {{args}}

image-win force="1": (_docker_build "tools/shared/docker/game-win/Dockerfile" "rrdash/trx-win" force)
image-linux force="1": (_docker_build "tools/shared/docker/game-linux/Dockerfile" "rrdash/trx-linux" force)
image-win-installer force="1": (_docker_build "tools/shared/docker/installer/Dockerfile" "rrdash/trx-installer" force)

push-image-linux: (image-linux "0") (_docker_push "rrdash/trx-linux")
push-image-win: (image-win "0") (_docker_push "rrdash/trx-win")

import "justfile.tr1"
import "justfile.tr2"

download-assets tr_version='all':
    tools/download_assets {{tr_version}}

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

[group('lint')]
lint-imports:
    tools/sort_imports

[group('lint')]
lint-format:
    pre-commit run -a

[group('lint')]
lint: (lint-imports) (lint-format)
