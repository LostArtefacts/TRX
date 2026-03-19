## Building on Linux

This is the building guide for Linux, using docker and just (the official way)

## 1- Installing the dependencies

First of all you'll have to install these dependencies:
- glew
- sdl2
- ffmpeg
- uthash
- pkgconfig
- meson
- Python3
- just

Install them with your distro's package manager (like apt, dnf, pacman, etc.)

Install also [docker desktop](http://docker.com/)

## 2- Building TRX

Before building TRX, run first `./tools/download_assets X`  <- Replace the "X" with TR version

Now you can run the building command with `just trx-build-linux target='debug'`

## 3- Running the game

Once you compiled the engine, you'll have to make the game work with its game files, so go to the root of the repo, go to `data/X/ship` (<- Replace "X" with TR version) and copy all the files in the build directory; also copy the game files from your original game copy

Now you can just run `./TRX` and the game will start