---
title: Injection
order: 42
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/inject.lua. Edit it there.
-->

## <a id="inject" name="inject"></a>Injection module

The content a mod brings with it: meshes, animations, sounds and other data.

A game flow names the injections its levels load. A script can name additional
injections, so a mod can ship its content without changing a game flow.

### Functions

- <a id="inject.declare" name="inject.declare"></a>[lua]`trx.inject.declare(declaration)`  
  Adds injections to every level, in addition to those named by its game flow.

  The function runs before each level loads its content. It can read the current
  settings and return a different list for each level.

  Return file names, not paths. A file beside the script is searched first. Other
  files are searched in the same order as game-flow injections: in the mod first,
  then in the base game. Use [`trx.path.resolve`](PATH.md#path.resolve) to check whether a file exists.

  Parameters:
  - <a id="inject.declare.declaration" name="inject.declare.declaration"></a>**`declaration`** (function). Called before each level loads and returns a list of file names.

  Example:
  ```lua
  trx.inject.declare(function()
    local files = { "mymod_models.bin" }
    if trx.config.get("visuals.enable_ps1_crystals") then
      files[#files + 1] = "wall_crystals.bin"
    end
    return files
  end)
  ```
