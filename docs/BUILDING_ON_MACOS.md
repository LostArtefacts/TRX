## Building on macOS

This guide describes the native macOS build workflow using Meson.

## Installing dependencies

Install either Homebrew or MacPorts, then install the required dependencies.

Homebrew:

```bash
brew install sdl2 glew ffmpeg@6 uthash pkgconfig meson python@3.14
```

MacPorts:

```bash
sudo port install sdl2 ffmpeg uthash pkgconfig glew meson python@3.14
```

## Building TRX

1. Download the shipped assets for the combined build:

    ```bash
    ./tools/download_assets --combined-only
    ```

2. Configure the build:

    Intel Macs:

    ```bash
    meson setup build src --prefix=/tmp/TRX.app --bindir=Contents/MacOS --buildtype release --cross-file tools/shared/mac/x86-64_cross_file.txt
    ```

    Apple Silicon Macs:

    ```bash
    meson setup build src --prefix=/tmp/TRX.app --bindir=Contents/MacOS --buildtype release
    ```

3. Build the project:

    ```bash
    meson install -C build --tags trx,common
    ```

## Other build methods

The native Meson workflow above is the recommended way to build TRX on macOS.

If you want to experiment with other approaches, such as Docker or a different
native setup, you are welcome to do so, but they are not part of the project's
official build workflow.

## Running the game

To prepare the game directory:

1. Use the installed app bundle from your Meson prefix.
2. Copy the original game files from your game installations into
   `Contents/Resources/games/<game-id>/`.

Once the files are in place, run:

```bash
./TRX.app/Contents/MacOS/TRX
```
