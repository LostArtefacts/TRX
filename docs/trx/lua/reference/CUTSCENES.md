---
title: Cutscenes
order: 13
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/cutscenes.lua. Edit it there.
-->

## Cutscenes module

    Module for TR4's in-game cutscenes, the animated scenes stored in
    `cutseq.pak` and started by a cutscene trigger. A cutscene plays once:
    the engine remembers which ones have run, and a script may consult or
    rewrite that memory. The cutscene levels of TR1-TR3, which the game flow
    lists and `/cut` plays, are a different thing: see `trx.game.cutscenes`.
  

### Properties

- **`trx.cutscenes.current`** (integer). Number of the cutscene playing, or `nil` if none is. *(read-only)*
- **`trx.cutscenes.is_playing`** (boolean). Whether a cutscene is on screen. *(read-only)*
- **`trx.cutscenes.fov`** (integer). Field of view a cutscene plays at, in the engine's own angle units. TR4 uses 11488, against 14560 for ordinary play.
- **`trx.cutscenes.letterbox`** (number). Depth of each cinematic bar, as a fraction of the screen height. `0` removes them.

### Functions

- [lua]`trx.cutscenes.play(num)`  
  Plays a cutscene, fading the scene out first. Does nothing if one is already playing or the game has no cutscene data.

  Parameters:
  - **`num`** (integer). Cutscene number.

  Example:
  ```lua
  trx.cutscenes.play(28)
  ```

- [lua]`trx.cutscenes.is_played(num)`  
  Whether a cutscene trigger naming this number has already been answered.

  Parameters:
  - **`num`** (integer). Cutscene number.

  Returns: boolean.

- [lua]`trx.cutscenes.set_played(num, played)`  
      Marks a cutscene as played or unplayed. Marking one as played keeps its
      trigger from firing; unmarking one lets it run again.

      A trigger may name a number the game has no cutscene for - TR4 uses 32 to
      ask for a full-motion video - and the engine remembers those the same way,
      so `on_cutscene_trigger` hears about each of them once. This is what clears
      that memory, and it takes any number a trigger may carry, not only the ones
      `play` accepts.


  Parameters:
  - **`num`** (integer). Cutscene number.
  - **`played`** (boolean). Whether it counts as played.

  Example:
  ```lua
  trx.cutscenes.set_played(7, true)
  ```

- [lua]`trx.cutscenes.forget_played()`  
  Forgets every cutscene, so all of them may run again.

- [lua]`trx.cutscenes.set_lara_return(pos, [rot])`  
      Places Lara where the next cutscene to end leaves her. A cutscene stands
      her at its own origin while it plays and puts her back where it found her
      afterwards; this says to put her somewhere else instead, as the original
      game does for the scenes that carry her along.

      It holds for one cutscene, whether named before `play` or while the scene
      runs, and is forgotten once she has been placed.


  Parameters:
  - **`pos`** (vec3). World position.
  - **`rot`** (integer, optional). Facing angle, in the engine's own angle units. Defaults to `0`.

  Example:
  ```lua
  trx.events.on_cutscene_start(function(num)
    if num == 12 then
      trx.cutscenes.set_lara_return({ x = 38912, y = 2048, z = 51200 })
    end
  end)
  ```
