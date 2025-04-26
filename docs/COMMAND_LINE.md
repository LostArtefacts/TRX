# Command line options

Currently the following command line interface options are available:

`-g/--gold` (legacy: `-gold`):
    Runs the Unfinished Business or the Golden Mask expansion pack, depending
    on the game.

`--demo-pc` (TR1X only, legacy: `-demo_pc`):
    Runs the PC demo level.

`-l/--level <path>`:
    Runs the game immediately launching it into the specified level.
    The path should be absolute. Internally, this option uses
    `TR*X_gameflow_level.json5` as a template instructing it how to run the
    game.

`-s/--save <num>`:
    Runs the game immediately loading a specific save slot. The first save
    starts at `num=1`. This option can be combined with `-l/--level`.
