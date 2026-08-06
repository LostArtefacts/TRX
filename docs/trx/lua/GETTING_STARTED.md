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
place. Nothing declares it: the file being there is what runs it. It is where a
game declares settings of its own, with `trx.config.declare`.

An expansion that has nothing of its own to set up needs no file: the script of
the game it extends runs instead. Shipping one replaces that script rather than
adding to it, so anything worth keeping goes in a module both require.

### Sharing code between scripts

A script can put what it has in common with another in a file of its own and
require it. A name carries the directory it lives in, so a call site says which
file it means:

```lua
local mine = require("tr1.my_module")            -- games/tr1/modules/my_module.lua
local nested = require("tr1.my_group.my_module") -- games/tr1/modules/my_group/my_module.lua
local other = require("tr1-ub.my_module")        -- any installed game, by its directory name
local pooled = require("common.my_module")       -- modules/my_module.lua, beside the engine
```

A module lives in `modules/`, alongside the `scripts/` the engine runs, and a
name reaches `modules/` alone: there is no name for a level script or for
`_game.lua`. `common` is the pool every game can reach, in `modules/` next to
the executable; create the directory if it is not there. A game's own directory
is named the way it appears in `games/`, so a script says which game it is
reaching into and gets the same file whichever game is running. `trx` is
reserved for the engine's own modules, which are the global `trx` table rather
than something to require.

A required script runs once, and every later call is handed what the first one
returned:

```lua
-- modules/my_module.lua
local M = {}
function M.greet() trx.log.info("hello from my_module") end
return M
```

Because each form names one directory, the same name in two of them is two
separate modules, and a game's own script is never mistaken for one it extends.
A script required by a level script belongs to that level: it runs again for
the next one, so anything it attaches is there for each level in turn. One
required by `_game.lua` runs once for the game. The first require of a name
decides which of the two it is, and every require after it is handed that
module, so the game and a level never hold two copies of one name.

Names hold letters, digits, `_`, `-` and the `.` that separates directories.
Case does not count, so a name spelled two ways is one module. There is no way
to name a file outside the directory the form points at.

### Interactive commands

The `/lua` console command also runs short Lua commands in-game:

```text
/lua
```
