# TR1Main

This is a dynamic library for the classic Tomb Raider I game (TombATI version).
The purpose of the library is to reimplement all the routines performed by the
game and enhance the gameplay with new options.

This project is inspired by Arsunt's [TR2Main](https://github.com/Arsunt/TR2Main/) project.

## Getting Started

For TR1Main to work, you will need a patched `tombati.exe` from
[here](https://github.com/rr-/TR1Main/tree/master/bin). Then you should
download TR1Main.dll from [releases](https://github.com/rr-/TR1Main/releases).
Both files should be put in your game folder. Then you can launch the game by
running the patched `tombati.exe`.

## Configuring

To configure TR1Main, copy
[TR1Main.json](https://raw.githubusercontent.com/rr-/TR1Main/master/TR1Main.json)
from this repository to your game folder, then edit it in your text editor such
as Notepad.

Currently the following configuration options are supported:

- `disable_medpacks`: hides all the medpacks (for No Meds challenge runs).
- `disable_healing_between_levels`: disables healing Lara between level reloads
  (for No Heal challenge runs).

## Building

- i686-w64-mingw32-gcc - C/C++ compiler
- Vim @ WSL - IDE

To compile the project with Docker, just run `./compile`.

## License

This project is licensed under the GNU General Public License - see the
[COPYING.md](COPYING.md) file for details.

## Copyright

(c) 2020 Marcin Kurczewski. All rights reserved. Original game is created by
Core Design Ltd. in 1996. Lara Croft and Tomb Raider are trademarks of Square
Enix Ltd.
