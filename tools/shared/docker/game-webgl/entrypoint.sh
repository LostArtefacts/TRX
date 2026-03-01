#!/usr/bin/env bash
# Docker entrypoint for WebGL/Emscripten builds.
#
# Usage:
#   docker run ... rrdash/trx-webgl build --target release --game tr1
set -euo pipefail

ACTION="${1:-build}"
shift || true

TARGET="debug"
GAME="tr1"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --target) TARGET="$2"; shift 2 ;;
        --game)   GAME="$2";   shift 2 ;;
        *)        shift ;;
    esac
done

case "$ACTION" in
    build)
        exec /app/tools/build_webgl.sh "$GAME" "$TARGET"
        ;;
    *)
        echo "Unknown action: $ACTION (expected: build)"
        exit 1
        ;;
esac
