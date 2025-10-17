---
title: Getting started
---

## Getting started

You'll need:

- A TR1X or TR2X build from the latest develop branch that adds Lua scripting.
- Familiarity with the game flow JSON format.

### Quick steps

Add per-level scripts in a level object:

```json5
{
    "levels": [
        // …,
        "script": "data/scripts/level1.lua",
        // …,
    ],
    // …
}
```

Create a file `data/scripts/level1.lua` folder in your project and put the
following content:

```lua
trx.events.on_level_load(function(level)
  trx.log.info("hello from level 1!")
end)
```

Start the game. In the logs, you should see the following:

```
INF | 2025-10-04 12:12:23.155 [cfg/tr1/level1.lua:4:?] hello from caves
```

---

Optionally, you can also load a global script by adding a global property to
your game flow configuration:

```json5
{
    // Optional global Lua script file
    "main_script": "data/scripts/global.lua",
}
```

### Interactive commands

You can also try out short lua commands in-game with the `/lua` console command:

```text
/lua
```
