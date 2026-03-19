## Building on macOS

Build instructions on macOS, using meson only


## 1- Installing a package manager and the dependencies

Install a package manager for installing the dependencies
Install MacPorts or Homebrew

Once installed one, you'll have to install these dependencies:

Homebrew: `brew install sdl2 glew ffmpeg@6 uthash pkgconfig meson python@3.14`

MacPorts: `sudo port install sdl2 ffmpeg uthash pkgconfig glew meson python@3.14`

## 2- Building TRX

Once you cloned the repo, download the game files with:

`./tools/download_assets X`  <- Replace the "X" with TR version

Once downloaded, you'll have to configure the project:

For Intel Macs: `meson setup build src --prefix=/tmp/TRX.app --bindir=Contents/MacOS --buildtype release --cross-file tools/shared/mac/x86-64_cross_file.txt`

For Apple Silicon Macs: `meson setup build src --prefix=/tmp/TRX.app --bindir=Contents/MacOS --buildtype release`

Once configured, build it: `meson compile -C build`

## 3- Running the game

Once you compiled the engine, you'll have to make the game work with its game files, so go to the root of the repo, go to `data/X/ship` (<- Replace "X" with TR version) and copy all the files in the build directory; also copy the game files from your original game copy

Now you can just run `./TRX` and the game will start