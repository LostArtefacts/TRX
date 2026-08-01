---
title: Getting started
order: 0
---

## Getting started

Scripting with Lua needs:

- A TR1 or TR2 TRX build from the latest develop branch that adds Lua scripting.
- Familiarity with the game flow JSON format.

### Quick steps

Add per-level scripts in a level object:

```json5
{
    "levels": [
        // …,
        {
            "script": "data/scripts/level1.lua",
            // …,
        },
        // …
    ],
    // …
}
```

Create a file `data/scripts/level1.lua` in the project and put the following
content:

```lua
trx.events.on_game_start(function(level)
  trx.log.info("hello from level 1!")
end)
```

Start the game. The logs should show the following:

```
INF | 2025-10-04 12:12:23.155 [data/scripts/level1.lua:2:?] hello from level 1!
```

---

A global script can be loaded as well, by adding a global property to the game
flow configuration:

```json5
{
    // Optional global Lua script file
    "main_script": "data/scripts/global.lua",
}
```

### Interactive commands

The `/lua` console command also runs short Lua commands in-game:

```text
/lua
```
