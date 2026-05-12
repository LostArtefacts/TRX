CWD := `pwd`
HOST_USER_UID := `id -u`
HOST_USER_GID := `id -g`
DOCKER_IMAGE_VERSION := "20260501.dev1"

default: (trx-build-win "debug")

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
        -e CCACHE_DIR \
        -e CCACHE_BASEDIR \
        -e CCACHE_COMPILERCHECK \
        -e CCACHE_MAXSIZE \
        -v {{CWD}}:/app/ \
        $full_tag \
        {{args}}

image-win force="1": (_docker_build "tools/shared/docker/game-win/Dockerfile" "rrdash/trx-win" force)
image-linux force="1": (_docker_build "tools/shared/docker/game-linux/Dockerfile" "rrdash/trx-linux" force)
image-win-installer force="1": (_docker_build "tools/shared/docker/installer/Dockerfile" "rrdash/trx-installer" force)

push-image-linux: (image-linux "0") (_docker_push "rrdash/trx-linux")
push-image-win: (image-win "0") (_docker_push "rrdash/trx-win")

download-assets tr_version='all':
    tools/download_assets {{tr_version}}

output-release-name:
    tools/output_release_name

output-current-version *args:
    tools/get_version {{args}}

output-current-changelog *args:
    tools/output_current_changelog {{args}}

output-package-name *args:
    tools/output_package_name {{args}}

clean:
    -find build/ -type f -delete
    -find tools/ -type f \( -ipath '*/out/*' -or -ipath '*/bin/*' -or -ipath '*/obj/*' \) -delete
    -find . -mindepth 1 -empty -type d -delete

[group('lint')]
lint-imports:
    tools/sort_imports

[group('lint')]
lint-format:
    prek -a

[group('lint')]
lint: (lint-imports) (lint-format)

trx-build-linux target='debug': (image-linux "0") (_docker_run "rrdash/trx-linux" "build" "--target" target)
trx-build-win target='debug': (image-win "0") (_docker_run "rrdash/trx-win" "build" "--target" target)

trx-build-win-installer target='release' *args: \
    (trx-build-win target) \
    (_docker_run "rrdash/trx-win" "package" "-o" "tools/installer/TRX_Installer/Resources/release.zip") \
    (image-win-installer "0") \
    (_docker_run "rrdash/trx-installer")

trx-package-linux target='debug' *args: (trx-build-linux target) (_docker_run "rrdash/trx-linux" "package" args)
trx-package-win target='debug' *args: (trx-build-win target) (_docker_run "rrdash/trx-win" "package" args)
trx-package-win-te artifact_path output *args:
    python3 tools/update_te_symlinks
    python3 tools/package_te_bundle.py --artifact {{artifact_path}} --output {{output}} {{args}}
trx-package-win-installer target='release' *args: \
    (trx-build-win-installer target args) \
    (_docker_run "rrdash/trx-win" "package" "--platform" "win-installer" args)
