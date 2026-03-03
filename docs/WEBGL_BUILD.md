# TRX WebGL Build Guide

This document describes how to build TRX (Tomb Raider engine) for the web using
Emscripten, producing a WebAssembly (WASM) + WebGL 2.0 build that runs in
modern browsers.

## Prerequisites

### 1. Emscripten SDK

Install and activate the Emscripten SDK (emsdk):

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh   # activate in current shell
```

### 2. Build Tools

- **Meson** >= 1.3.0
- **Ninja** (build backend)
- **Pillow** (Python, for PWA icon generation)

```bash
pip install meson ninja Pillow
```

### 3. Dependencies

The WebGL build uses Emscripten's built-in ports for most dependencies:

| Dependency  | Status               | Notes                                   |
|-------------|----------------------|-----------------------------------------|
| SDL2        | Emscripten port      | `-sUSE_SDL=2`                           |
| zlib        | Emscripten port      | `-sUSE_ZLIB=1`                          |
| OpenGL ES 3 | Built-in            | `-sFULL_ES3=1` (WebGL 2.0)             |
| GLEW        | Not needed           | GL ES headers provided by Emscripten    |
| Lua         | Must provide headers | Build from source with `emcc` or skip   |
| PCRE2       | Optional             | Build from source with `emcc` or skip   |
| FFmpeg      | Not available        | Replaced by lightweight C decoders + HTML5 video |

## Building

### Quick Build (local Emscripten SDK)

```bash
source /path/to/emsdk/emsdk_env.sh

./tools/build_webgl.sh tr1 debug            # TR1 debug build
./tools/build_webgl.sh tr2 release          # TR2 release build
./tools/build_webgl.sh tr1 release --no-game-data   # without bundled game data
./tools/build_webgl.sh tr1 debug --eruda    # with eruda mobile debugger
```

Usage: `./tools/build_webgl.sh [tr1|tr2] [debug|release|debugoptim] [options]`

Options:

| Flag              | Effect                                              |
|-------------------|-----------------------------------------------------|
| `--no-game-data`  | Do not bundle user game data even if present         |
| `--eruda`         | Inject eruda mobile debugger into the HTML           |

### Docker Build (recommended for CI / reproducibility)

A Docker image based on `emscripten/emsdk` provides a self-contained build
environment. The `deploy.sh` script at the repository root automates building
both TR1 and TR2, deploying to a local directory, and starting HTTP servers:

```bash
# Test deployment (ports 8081/8082, bundles game data, includes eruda)
sudo ./deploy.sh test

# Test deployment without game data (users upload at runtime)
sudo ./deploy.sh test --no-game-data

# Production deployment (ports 9081/9082, no game data, no eruda)
sudo ./deploy.sh prod
```

| Environment | Ports         | Game data default | Eruda |
|-------------|---------------|-------------------|-------|
| `test`      | 8081 / 8082   | bundled           | yes   |
| `prod`      | 9081 / 9082   | excluded          | no    |

Both defaults can be overridden with `--game-data` or `--no-game-data`.

The Docker image (`tools/shared/docker/game-webgl/Dockerfile`) installs
Meson, Ninja, pyjson5, and Pillow on top of the official emsdk image. The
entrypoint (`tools/shared/docker/game-webgl/entrypoint.sh`) forwards
`--no-game-data` and `--eruda` flags to `build_webgl.sh`.

### Manual Build

```bash
source /path/to/emsdk/emsdk_env.sh

meson setup \
  --cross-file tools/shared/emscripten/emscripten_cross.ini \
  --buildtype debug \
  -Dstaticdeps=false \
  -Dgame=tr1 \
  -Dwebgl_bundle_gamedata=auto \
  build/webgl-tr1 \
  src/

meson compile -C build/webgl-tr1 TRX
```

The `webgl_bundle_gamedata` option controls whether user game data (levels,
music, SFX) is embedded in `TRX.data`:

| Value  | Behavior                                         |
|--------|--------------------------------------------------|
| `auto` | Bundle if game data directories exist (default)  |
| `yes`  | Always bundle (error if directories are missing) |
| `no`   | Never bundle; users upload at runtime            |

## Output Files

After a successful build:

```
build/webgl-tr1/
├── TRX.html              # Main HTML page
├── TRX.js                # Emscripten JavaScript glue code
├── TRX.wasm              # WebAssembly binary
├── TRX.data              # Preloaded assets (always includes TRX config/shaders;
│                         #   includes game data only if bundled)
├── gamedata.js           # Game data upload/persistence manager
├── vendor/
│   └── fflate.min.js     # ZIP/gzip decompression library
├── manifest.webmanifest  # PWA manifest
├── sw.js                 # Service worker for offline support
├── icon-192.png          # PWA icon (192x192)
├── icon-512.png          # PWA icon (512x512)
└── fmv/                  # FMV cutscenes (if tr1_data/fmv/ exists)
    ├── cafe.mp4
    └── ...
```

The build script applies cache-busting query strings (based on the WASM hash)
to all `<script>` tags in `TRX.html`, ensuring browsers never serve stale
assets after a rebuild.

## Running Locally

WebGL builds require a web server (WASM cannot be loaded from `file://` URLs):

```bash
cd build/webgl-tr1
python3 -m http.server 8080
# Open http://localhost:8080/TRX.html
```

## Game Data

### What TRX ships (always bundled in TRX.data)

- `cfg/` — configuration JSON5 (strings, UI, gameflow, catalogs)
- `shaders/` — GLSL shaders
- `data/injections/` — binary patches
- `data/images/` — UI artwork (WebP)
- `data/scripts/` — Lua scripts

### What users must provide

These are the copyrighted game files from GOG or Steam:

- **TR1**: `data/*.phd` (levels), `music/Track*.flac` (59 tracks)
- **TR2**: `data/*.tr2` (levels), `data/main.sfx`, `music/*.mp3`

FMV cutscenes (`.mp4`) are optional; the game skips missing cutscenes
gracefully.

### Build with game data (default)

Place game files in `tr1_data/` or `tr2_data/` alongside the repository:

```
tr1_data/
├── data/          # Level files (.phd, .sfx, etc.)
├── music/         # Music tracks (Track02.flac .. Track60.flac)
└── fmv/           # FMV cutscenes as .mp4 (optional)
```

When game data is bundled, the runtime flow is:

1. Browser loads TRX.html, TRX.js, TRX.wasm, TRX.data (includes game data)
2. Emscripten FS ready with game data already in `/data/` and `/music/`
3. Game starts immediately
4. In the background, game data is copied to IndexedDB for offline support
5. On subsequent visits, data loads from IndexedDB (fast, works offline)

### Build without game data (`--no-game-data`)

When game data is not bundled, users see an upload screen on first visit:

1. Browser loads the (smaller) TRX.data containing only TRX-owned assets
2. Upload UI appears asking the user for their game files
3. User uploads via folder selection, ZIP, or tar.gz
4. Files are extracted, mapped to the correct VFS paths, and stored in IndexedDB
5. Game starts
6. On subsequent visits, data loads from IndexedDB without re-uploading

### Upload formats

The upload UI (`gamedata.js`) accepts:

- **Folder** — via the browser's directory picker
- **ZIP archive** — extracted with fflate (~30KB, MIT license)
- **tar.gz archive** — extracted with fflate (gzip) + minimal tar parser

The uploader auto-detects the archive root, normalizes paths to lowercase, and
maps files to the expected VFS locations (`/data/`, `/music/`, etc.) regardless
of the original directory structure.

### IndexedDB persistence

Game data is stored in an IndexedDB database named `trx-gamedata-{game_id}`
(e.g., `trx-gamedata-tr1`) with two object stores:

- `files` — keyed by VFS path (e.g., `data/gym.phd`), values are ArrayBuffers
- `meta` — stores upload metadata (file count, total bytes, upload date)

## FMV Cutscenes

FFmpeg cannot be compiled for the web platform, so the desktop Video API is not
available. Instead, the WebGL build plays FMVs through the browser's native
HTML5 `<video>` element.

### Preparing FMV files

Convert original FMV files to H.264 MP4:

```bash
ffmpeg -i upscaled.ogv -i original.fmv \
  -map 0:v -map 1:a \
  -c:v libx264 -preset medium -crf 23 -profile:v main -pix_fmt yuv420p \
  -c:a aac -b:a 128k -movflags +faststart \
  output.mp4
```

Place the results in `tr1_data/fmv/` with lowercase `.mp4` filenames.

### Runtime playback

The Emscripten FMV implementation (`fmv_emscripten.c`) creates a Blob URL from
the video data and plays it via a hidden `<video>` element. Each frame is
uploaded to a WebGL texture with `texImage2D(videoElement)` and rendered through
the `GFX_2D_Renderer` pipeline with letterbox fitting.

For builds without bundled game data, user-uploaded MP4 files are stored in
IndexedDB and loaded as blob URLs on demand. If no blob is available, the
player falls back to fetching via HTTP (for builds that include FMVs in the
`fmv/` directory).

`File_GuessExtension()` tries `.mp4` first, so the `.avi` paths in
`gameflow.json5` do not need to be changed.

## PWA Support

The build produces a Progressive Web App that can be installed on desktop and
mobile devices:

- **`manifest.webmanifest`** — generated from `manifest.webmanifest.in` with
  game-specific names (TR1X, TR2X). Configures fullscreen landscape display.
- **`sw.js`** — service worker generated from `sw.js.in`. Precaches all static
  assets (HTML, JS, WASM, gamedata.js, fflate, icons). Caches `TRX.data` on
  first use for offline support. FMV streaming requests are passed through
  uncached.
- **Icons** — generated from `data/trx/icon.png` by `generate_icons.py`
  (requires Pillow). Produces 192x192 and 512x512 PNGs.

After the first successful load (whether from preloaded data or user upload),
the app works fully offline.

## Loading Screen

The loading screen shows a two-phase progress bar:

1. **Download phase (0-50%)** — downloading TRX.wasm and TRX.data
2. **Data loading phase (50-100%)** — loading game data from IndexedDB into
   the virtual filesystem

The loading screen remains visible until the engine is ready to display the
start gate ("press any key" splash), preventing any black screen gap.

## Debugging

### Eruda mobile debugger

The `--eruda` flag injects the [eruda](https://github.com/nicknisi/eruda)
mobile console into the build. This provides a floating developer tools panel
useful for debugging on mobile devices where browser DevTools are unavailable.

When using `deploy.sh`, test builds include eruda automatically; production
builds do not.

### Browser DevTools

Use the browser's built-in developer tools for debugging. The Sources panel
shows the WASM module and can set breakpoints in the JavaScript glue code.
Native C stack traces are not available in WebGL builds.

## Architecture Notes

### Audio

FFmpeg is not available on the web platform. The WebGL build uses four
lightweight, header-only C audio decoders that compile natively to
WebAssembly:

| Library        | Format    | License       | Purpose                     |
|----------------|-----------|---------------|-----------------------------|
| `dr_wav.h`     | WAV/RIFF  | Public domain | Sound effects + WAV music   |
| `dr_flac.h`    | FLAC      | Public domain | Music streaming             |
| `dr_mp3.h`     | MP3       | MIT           | Music streaming             |
| `stb_vorbis.c` | OGG Vorbis| Public domain | Music streaming             |

These live in `src/trx/engine/vendor/` and are compiled into two
Emscripten-specific source files:

- `audio_sample_emscripten.c` — decodes sound effects (WAV) via `dr_wav`,
  with `SDL_AudioStream` for resampling to 44100 Hz mono.
- `audio_stream_emscripten.c` — streams music in any of the four formats,
  auto-detected by magic bytes, resampled via `SDL_AudioStream`.

The SDL2 audio device and mixer callback (`audio.c`) work unchanged on
Emscripten.

### Images

The native build uses FFmpeg to decode images (title screen, loading screens,
credits). The WebGL build replaces this with:

| Library              | Formats                  | Notes                          |
|----------------------|--------------------------|--------------------------------|
| `stb_image.h`        | PNG, JPEG, BMP, GIF, TGA | Header-only, in `vendor/`      |
| `stb_image_resize2.h`| —                        | Image scaling (crop/letterbox) |
| `stb_image_write.h`  | PNG, JPEG                | Screenshot saving              |
| libwebp (decode)     | WebP                     | Pre-built static library       |

The game's background images are distributed as WebP files. Since `stb_image`
does not support WebP, the image loader tries `stb_image` first and falls back
to `libwebp` for WebP decoding.

The pre-built `libwebpdecoder.a` (compiled from Google's libwebp 1.5.0 source
with `emcc`) is checked into the repository at `src/trx/engine/vendor/`. To
rebuild it from source:

```bash
source /path/to/emsdk/emsdk_env.sh
wget https://storage.googleapis.com/downloads.webmproject.org/releases/webp/libwebp-1.5.0.tar.gz
tar xzf libwebp-1.5.0.tar.gz && cd libwebp-1.5.0
mkdir build_wasm && cd build_wasm
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DWEBP_BUILD_ANIM_UTILS=OFF -DWEBP_BUILD_CWEBP=OFF \
  -DWEBP_BUILD_DWEBP=OFF -DWEBP_BUILD_GIF2WEBP=OFF \
  -DWEBP_BUILD_IMG2WEBP=OFF -DWEBP_BUILD_VWEBP=OFF \
  -DWEBP_BUILD_WEBPINFO=OFF -DWEBP_BUILD_WEBPMUX=OFF \
  -DWEBP_BUILD_EXTRAS=OFF -DWEBP_BUILD_LIBWEBPMUX=OFF
emmake make -j$(nproc) webpdecoder
cp libwebpdecoder.a /path/to/TRX/src/trx/engine/vendor/
```

### Graphics Pipeline

The WebGL build targets **WebGL 2.0** (OpenGL ES 3.0), which is the closest
match to the desktop OpenGL 3.3 Core Profile used by TRX.

Key adaptations:
- Shaders are compiled as **GLSL ES 3.00** (`#version 300 es`) instead of
  GLSL 3.30 (`#version 330 core`)
- `precision highp float/int` qualifiers are injected automatically
- Desktop-only calls (`glPolygonMode`, `glDrawBuffer`, `glMapBuffer`, etc.)
  are replaced with ES 3.0 equivalents or no-ops
- GLEW is replaced by direct GL ES 3 headers from Emscripten

### Compatibility Layer

The file `src/trx/gfx/gl/gl_webgl_compat.h` provides a compatibility shim
that maps desktop GL concepts to their WebGL/ES equivalents:

- `glClearDepth()` -> `glClearDepthf()`
- `glMapBuffer()` -> `glMapBufferRange()`
- `glBindFragDataLocation()` -> no-op (use `layout(location=0)` in shaders)
- `glPolygonMode()` -> no-op (wireframe not available in WebGL)
- GLEW `glewInit()` -> no-op

### Shader Compatibility

All GLSL shaders have been made compatible with both GLSL 3.30 (desktop) and
GLSL ES 3.00 (WebGL 2) by:

- Using `float()` casts for integer constants in `clamp()`/`max()` calls
- Ensuring `texture()` calls use consistent types
- Avoiding `int * float` implicit conversions
- The preprocessor in `program.c` automatically injects the correct version
  string and precision qualifiers based on the build target

## Key Source Files

| File | Purpose |
|------|---------|
| `tools/build_webgl.sh` | Main build script |
| `tools/shared/emscripten/shell.html` | HTML shell template (loading UI, upload UI, canvas) |
| `tools/shared/emscripten/gamedata.js` | Game data upload, extraction, IndexedDB persistence |
| `tools/shared/emscripten/emscripten_cross.ini` | Meson cross-compilation file |
| `tools/shared/emscripten/vendor/fflate.min.js` | ZIP/gzip decompression (MIT) |
| `tools/shared/emscripten/sw.js.in` | Service worker template |
| `tools/shared/emscripten/manifest.webmanifest.in` | PWA manifest template |
| `tools/shared/emscripten/generate_icons.py` | PWA icon generator |
| `tools/shared/docker/game-webgl/Dockerfile` | Docker build image |
| `tools/shared/docker/game-webgl/entrypoint.sh` | Docker entrypoint |
| `src/trx/gfx/gl/gl_webgl_compat.h` | OpenGL ES compatibility shim |
| `src/trx/game/fmv_emscripten.c` | FMV playback via HTML5 video |

## Known Limitations

1. **No wireframe mode** — `glPolygonMode` is not available in WebGL.
2. **Threading** — the build uses Emscripten's Asyncify for cooperative
   multitasking instead of true threads (see `docs/WEBGL_ASYNCIFY.md`).
3. **Backtraces** — native stack traces are not available; use browser
   developer tools for debugging.
4. **Large download** — music and FMV files add significant size. Building
   with `--no-game-data` avoids this by having users upload their own files.
5. **FMV format** — only H.264 MP4 is supported (browser-native decoding).
   Original `.rpl`/`.fmv` formats cannot be played.

## Browser Requirements

- **WebGL 2.0** support (OpenGL ES 3.0)
- **WebAssembly** support
- **IndexedDB** support (for game data persistence)
- Modern browsers: Chrome 56+, Firefox 51+, Safari 15+, Edge 79+
