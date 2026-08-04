---
title: Getting started
order: 0
---

## Getting started

Scripting with Lua needs:

- A TR1 or TR2 TRX build from the latest develop branch that adds Lua scripting.
- Familiarity with the game flow JSON format.

### Quick steps

A level's script is named after the level it belongs to. For a level that loads
`level1.phd`, create `scripts/level1.lua` in the game's directory and put the
following content in it:

```lua
trx.events.on_game_start(function()
  trx.log.info("hello from level 1!")
end)
```

Nothing declares the file: the game flow names the level, and the script beside
it under that name runs as the level loads.

Start the game. The logs should show the following:

```
INF | 2025-10-04 12:12:23.155 [scripts/level1.lua:2:?] hello from level 1!
```

---

### The game's own script

A game can also ship one script that belongs to the game rather than to any of
its levels. Create `scripts/_game.lua` in the game's directory and it runs once
as the game starts, with the game flow, the strings and the settings all in
place. Nothing declares it: the file being there is what runs it.

An expansion that has nothing of its own to set up needs no file: the script of
the game it extends runs instead. Shipping one replaces that script rather than
adding to it, so anything worth keeping goes in a module both require.

### Interactive commands

The `/lua` console command also runs short Lua commands in-game:

```text
/lua
```
