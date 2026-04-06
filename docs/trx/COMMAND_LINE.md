---
title: CLI options
order: 1
---

# Command line options

Currently the following command line interface options are available:

- `--mod <MOD_ID>`  
  Runs a specific game or mod directly.
  Available mods:
  - `tr1` (Tomb Raider I)
  - `tr1-ub` (Tomb Raider I: Unfinished Business)
  - `tr1-demo-pc` (Tomb Raider I: PC Demo)
  - `tr2` (Tomb Raider II)
  - `tr2-gm` (Tomb Raider II: The Golden Mask)
  - `tr3` (Tomb Raider III)
  - `tr3-la` (Tomb Raider III: The Lost Artifact)

  TRX remembers the last selected game or expansion when no explicit startup
  option is given. Because of that, `TRX.exe` starts whichever game or
  expansion was selected most recently. Use `TRX.exe --mod` to force the
  matching expansion pack from a shortcut.

- `-l <path|num>`, `--level <path|num>`  
  Runs the game immediately launching it into the specified level. If `<path>`
  is provided, runs the custom level located in the specified location, which
  should be absolute. Internally, this option uses `tr*-level/gameflow.json5`
  as a template instructing it how to run the game. If `<num>` is an integer,
  plays the level with the given number within the main game flow (1-based).

- `-s <num>`, `--save <num>`:  
  Runs the game immediately loading a specific save slot. The first save starts
  at `num=1`. This option can be combined with `-l`/`--level`.

- `--test-record <PATH>`:  
  Records gameplay events to an external text file.

  `--test-replay <PATH>`, `--test-play <PATH>`:  
  Replays gameplay events from an external text file.

  `--headless`:  
  Runs the game in command line only. Only available with `--test-replay`.

- `--headless-fps <num>`:  
  In headless mode, forces the simulation to run at a constant FPS. If omitted
  or zero, uses the FPS from the configuration.

- `--debug-render-performance`:  
  Outputs diagnostic information related to GPU usage and throughput.

- `-q`, `--quiet`  
  Suppresses most of output to the standard output, keeping only errors.
  The log file is written to normally.

> [!TIP]
> If you want `TRX.exe` to start the main game again after using an expansion
> shortcut, switch back to the main game in the passport first, or launch it
> with `--mod`/`--engine` from the shortcut as needed.

> [!NOTE]
> Gameplay capture is considered an internal testing tool, and may be
> subject to breaking changes without warnings.

# Legacy command line options

- `-g`, `--gold` (legacy: `-gold`)  
  Runs the Unfinished Business or the Golden Mask expansion pack, depending
  on the last launched game. Please use `--mod` option instead.

- `--demo-pc` (legacy: `-demo_pc`) (TR1 only)  
  Runs the PC demo level. Please use `--mod` option instead.
