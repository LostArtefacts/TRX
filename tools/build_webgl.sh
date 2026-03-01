#!/usr/bin/env bash
#
# TRX WebGL Build Script
#
# Builds TRX for the web using Emscripten (WASM + WebGL 2).
#
# Prerequisites:
#   1. Install Emscripten SDK: https://emscripten.org/docs/getting_started/downloads.html
#      git clone https://github.com/emscripten-core/emsdk.git
#      cd emsdk && ./emsdk install latest && ./emsdk activate latest
#
#   2. Activate the SDK in your shell:
#      source /path/to/emsdk/emsdk_env.sh
#
#   3. Install Meson (>= 1.3.0) and Ninja:
#      pip install meson ninja
#
#
# Usage:
#   ./tools/build_webgl.sh [tr1|tr2] [debug|release|debugoptim]
#
# Output:
#   build/webgl-<game>/TRX.html
#   build/webgl-<game>/TRX.js
#   build/webgl-<game>/TRX.wasm
#   build/webgl-<game>/TRX.data  (preloaded game data)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
CROSS_FILE="$PROJECT_ROOT/tools/shared/emscripten/emscripten_cross.ini"

GAME="${1:-tr1}"
BUILD_TYPE="${2:-debug}"
BUILD_DIR="$PROJECT_ROOT/build/webgl-${GAME}"

echo "============================================"
echo "  TRX WebGL Build"
echo "  Game: $GAME"
echo "  Build type: $BUILD_TYPE"
echo "============================================"

# Validate game selection
case "$GAME" in
    tr1|tr2) ;;
    *)
        echo "Unknown game: $GAME (expected: tr1, tr2)"
        exit 1
        ;;
esac

# Verify Emscripten is available
if ! command -v emcc &>/dev/null; then
    echo "ERROR: emcc not found in PATH."
    echo "Please install and activate the Emscripten SDK first:"
    echo "  source /path/to/emsdk/emsdk_env.sh"
    exit 1
fi

echo "Using Emscripten: $(emcc --version | head -1)"

# Build type → Meson buildtype
case "$BUILD_TYPE" in
    debug)
        MESON_BUILDTYPE="debug"
        ;;
    release)
        MESON_BUILDTYPE="release"
        ;;
    debugoptim)
        MESON_BUILDTYPE="debugoptimized"
        ;;
    *)
        echo "Unknown build type: $BUILD_TYPE (expected: debug, release, debugoptim)"
        exit 1
        ;;
esac

# Setup or reconfigure
if [ ! -f "$BUILD_DIR/build.ninja" ]; then
    echo ""
    echo ">>> Configuring Meson build..."
    meson setup \
        --cross-file "$CROSS_FILE" \
        --buildtype "$MESON_BUILDTYPE" \
        -Dstaticdeps=false \
        -Dgame="$GAME" \
        "$BUILD_DIR" \
        "$PROJECT_ROOT/src/"
else
    echo ""
    echo ">>> Reconfiguring existing build..."
    meson configure \
        --buildtype "$MESON_BUILDTYPE" \
        -Dgame="$GAME" \
        "$BUILD_DIR"
fi

echo ""
echo ">>> Compiling..."
meson compile -C "$BUILD_DIR" TRX

# Copy FMV cutscenes alongside the build output so they can be streamed
# on demand via HTTP (they are not embedded in TRX.data).
FMV_SRC="$PROJECT_ROOT/${GAME}_data/fmv"
if [ -d "$FMV_SRC" ]; then
    echo ""
    echo ">>> Copying FMV files for HTTP streaming..."
    mkdir -p "$BUILD_DIR/fmv"
    cp -u "$FMV_SRC"/*.mp4 "$BUILD_DIR/fmv/" 2>/dev/null || true
fi

# Add cache-busting query strings to the built HTML so that browsers
# and reverse proxies (nginx, CDNs) never serve stale .js/.wasm/.data.
CACHE_BUST="v=$(md5sum "$BUILD_DIR/TRX.wasm" | cut -c1-8)"
echo ""
echo ">>> Cache-busting: $CACHE_BUST"
sed -i "s|src=\"TRX.js\"|src=\"TRX.js?${CACHE_BUST}\"|" "$BUILD_DIR/TRX.html"
sed -i "s|var _trxCacheBust = '';  // __CACHE_BUST__|var _trxCacheBust = '${CACHE_BUST}';|" "$BUILD_DIR/TRX.html"

# --- PWA assets ---
SHARED_EMC="$PROJECT_ROOT/tools/shared/emscripten"

# Game display names
case "$GAME" in
    tr1) APP_NAME="TR1X"; SHORT_NAME="TR1X" ;;
    tr2) APP_NAME="TR2X"; SHORT_NAME="TR2X" ;;
esac

echo ""
echo ">>> Generating PWA manifest..."
sed -e "s|__APP_NAME__|${APP_NAME}|g" \
    -e "s|__SHORT_NAME__|${SHORT_NAME}|g" \
    "$SHARED_EMC/manifest.webmanifest.in" > "$BUILD_DIR/manifest.webmanifest"

echo ">>> Generating service worker..."
sed -e "s|__APP_NAME__|${APP_NAME}|g" \
    -e "s|__SHORT_NAME__|${SHORT_NAME}|g" \
    -e "s|__CACHE_BUST__|${CACHE_BUST}|g" \
    "$SHARED_EMC/sw.js.in" > "$BUILD_DIR/sw.js"

echo ">>> Generating PWA icons..."
ICON_SRC="$PROJECT_ROOT/data/trx/icon.png"
python3 "$SHARED_EMC/generate_icons.py" "$ICON_SRC" "$BUILD_DIR"

echo ""
echo "============================================"
echo "  Build complete! ($GAME)"
echo ""
echo "  Output files:"
echo "    $BUILD_DIR/TRX.html"
echo "    $BUILD_DIR/TRX.js"
echo "    $BUILD_DIR/TRX.wasm"
echo "    $BUILD_DIR/fmv/    (streamed on demand)"
echo ""
echo "  To test locally:"
echo "    cd $BUILD_DIR"
echo "    python3 -m http.server 8080"
echo "    # Open http://localhost:8080/TRX.html"
echo "============================================"
