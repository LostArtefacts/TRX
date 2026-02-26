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

```bash
pip install meson ninja
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

### 4. Game Data

Place the TR1 game data (from Steam, GOG, etc.) in `tr1_data/`:

```
tr1_data/
├── data/          # Level files (.phd, .sfx, etc.)
├── music/         # Music tracks (Track02.flac .. Track60.flac)
└── fmv/           # FMV cutscenes as .mp4 (see "FMV Cutscenes" below)
```

The `music/` directory is not included with the Steam release. You must
provide FLAC, OGG, MP3, or WAV music files separately. Name them
`Track02.flac` through `Track60.flac` (or the equivalent extension).

## Quick Build

```bash
# Activate Emscripten SDK
source /path/to/emsdk/emsdk_env.sh

# Run the build script
./tools/build_webgl.sh debug     # debug build
./tools/build_webgl.sh release   # optimized release build
```

## Manual Build

```bash
# Activate Emscripten SDK
source /path/to/emsdk/emsdk_env.sh

# Configure
meson setup \
  --cross-file tools/shared/emscripten/emscripten_cross.ini \
  --buildtype debug \
  -Dstaticdeps=false \
  build/webgl \
  src/

# Build
meson compile -C build/webgl TRX
```

## Output Files

After a successful build, you'll find:

```
build/webgl/
├── TRX.html    # Main HTML page (uses shell template)
├── TRX.js      # Emscripten JavaScript glue code
├── TRX.wasm    # WebAssembly binary
└── TRX.data    # Preloaded game data + music
```

## Running Locally

WebGL builds require a web server due to browser security restrictions
(WASM cannot be loaded from `file://` URLs):

```bash
cd build/webgl
python3 -m http.server 8080
# Open http://localhost:8080/TRX.html in your browser
```

## Browser Requirements

- **WebGL 2.0** support (OpenGL ES 3.0)
- **WebAssembly** support
- Modern browsers: Chrome 56+, Firefox 51+, Safari 15+, Edge 79+

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
credits). The WebGL build replaces this with two libraries:

| Library              | Formats                  | Notes                          |
|----------------------|--------------------------|--------------------------------|
| `stb_image.h`        | PNG, JPEG, BMP, GIF, TGA | Header-only, in `vendor/`      |
| `stb_image_resize2.h`| —                        | Image scaling (crop/letterbox) |
| `stb_image_write.h`  | PNG, JPEG                | Screenshot saving              |
| libwebp (decode)     | WebP                     | Pre-built static library       |

The game's background images (title, loading screens, credits) are distributed
as WebP files. Since `stb_image` does not support WebP, the image loader tries
`stb_image` first and falls back to `libwebp` for WebP decoding.

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

### FMV Cutscenes

FFmpeg cannot be compiled for the web platform, so the desktop Video\_\* API
(which wraps libavcodec/libavformat) is not available. Instead, the WebGL
build plays FMV cutscenes through the browser's native HTML5 `<video>`
element:

1. At **build time**, convert original FMV files to H.264 MP4 using the
   system's FFmpeg. Place the results in `tr1_data/fmv/`:

   ```bash
   # Example: convert original .fmv files (PSX MDEC + XA audio) to MP4.
   # If you have AI-upscaled video (e.g. OGV) and originals with audio,
   # mux the upscaled video with the original audio:
   ffmpeg -i upscaled.ogv -i original.fmv \
     -map 0:v -map 1:a \
     -c:v libx264 -preset medium -crf 23 -profile:v main -pix_fmt yuv420p \
     -c:a aac -b:a 128k -movflags +faststart \
     output.mp4
   ```

   Expected filenames (lowercase, `.mp4` extension):

   ```
   tr1_data/fmv/
   ├── core.mp4      # Legal splash (Core Design)
   ├── escape.mp4    # Legal splash (Eidos)
   ├── cafe.mp4      # Intro cinematic
   ├── mansion.mp4   # Lara's Home outro
   ├── snow.mp4      # Before Caves
   ├── lift.mp4      # Before St. Francis' Folly
   ├── vision.mp4    # Before City of Khamoon
   ├── canyon.mp4    # Before Natla's Mines
   ├── pyramid.mp4   # Before Atlantis
   ├── prison.mp4    # During Atlantis
   └── end.mp4       # During The Great Pyramid
   ```

2. At **run time**, the Emscripten FMV implementation (`fmv.c`) reads the
   MP4 from the virtual filesystem, creates a Blob URL, and plays it via a
   hidden `<video>` element. Each frame is uploaded to a WebGL texture with
   `texImage2D(videoElement)` and rendered through the existing
   `GFX_2D_Renderer` pipeline with letterbox fitting. Audio is played by
   the browser natively. The player can skip cutscenes with Escape or Enter,
   matching the desktop behaviour.

   `File_GuessExtension()` tries `.mp4` first, so as long as the converted
   files exist the game finds them automatically — the `.avi` paths in
   `gameflow.json5` do not need to be changed.

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

- `glClearDepth()` → `glClearDepthf()`
- `glMapBuffer()` → `glMapBufferRange()`
- `glBindFragDataLocation()` → no-op (use `layout(location=0)` in shaders)
- `glPolygonMode()` → no-op (wireframe not available in WebGL)
- GLEW `glewInit()` → no-op

### Known Limitations

1. **No wireframe mode** – `glPolygonMode` is not available in WebGL.
2. **File I/O** – Game data must be preloaded into Emscripten's virtual
   filesystem (MEMFS) at build time or fetched asynchronously.
3. **Threading** – The build uses Emscripten's Asyncify for cooperative
   multitasking instead of true threads.
4. **Backtraces** – Native stack traces are not available; use browser
   developer tools for debugging.
5. **Large download** – Music and FMV files add significant size to the
   data bundle. Transcoding music from FLAC to OGG Vorbis or MP3 before
   building can help reduce this.

## Shader Compatibility

All GLSL shaders have been made compatible with both GLSL 3.30 (desktop) and
GLSL ES 3.00 (WebGL 2) by:

- Using `float()` casts for integer constants in `clamp()`/`max()` calls
- Ensuring `texture()` calls use consistent types
- Avoiding `int * float` implicit conversions
- The preprocessor in `program.c` automatically injects the correct version
  string and precision qualifiers based on the build target
