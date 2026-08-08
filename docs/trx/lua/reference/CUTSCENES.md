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

## <a id="cutscenes" name="cutscenes"></a>Cutscenes module

Module for TR4's in-game cutscenes, the animated scenes stored in
`cutseq.pak` and started by a cutscene trigger. A cutscene plays once:
the engine remembers which ones have run, and a script may consult or
rewrite that memory. The cutscene levels of TR1-TR3, which the game flow
lists and `/cut` plays, are a different thing: see [`trx.game.cutscenes`](GAME.md#game.cutscenes).

### Properties

- <a id="cutscenes.current" name="cutscenes.current"></a>**`trx.cutscenes.current`** ([trx.cutscenes.Num](#cutscenes.Num)). Number of the cutscene playing, or `nil` if none is. *(read-only)*
- <a id="cutscenes.frame_num" name="cutscenes.frame_num"></a>**`trx.cutscenes.frame_num`** ([trx.cutscenes.FrameNum](#cutscenes.FrameNum)). Which frame of the running cutscene is on screen, or `nil` if none is
  running. A cutscene's actors are animation tracks rather than items, so
  nothing in it can be triggered or listened to; naming a frame is how a
  script acts part-way through one, as the original game does. *(read-only)*
- <a id="cutscenes.is_playing" name="cutscenes.is_playing"></a>**`trx.cutscenes.is_playing`** (boolean). Whether a cutscene is on screen. *(read-only)*
- <a id="cutscenes.fov" name="cutscenes.fov"></a>**`trx.cutscenes.fov`** ([trx.math.Angle](MATH.md#math.Angle)). Field of view a cutscene plays at. TR4 uses 11488, against 14560 for ordinary play.
- <a id="cutscenes.letterbox" name="cutscenes.letterbox"></a>**`trx.cutscenes.letterbox`** (number). Depth of each cinematic bar, as a fraction of the screen height. `0` removes them.

### Structures

- <a id="cutscenes.Num" name="cutscenes.Num"></a>[lua]`trx.cutscenes.Num`

    Cutscene number, as a cutscene trigger names it. Counted from 0.

- <a id="cutscenes.FrameNum" name="cutscenes.FrameNum"></a>[lua]`trx.cutscenes.FrameNum`

    A frame's number within the cutscene it belongs to. Counted from 0.

### Functions

- <a id="cutscenes.play" name="cutscenes.play"></a>[lua]`trx.cutscenes.play(num)`  
  Plays a cutscene, fading the scene out first. Does nothing if one is already playing or the game has no cutscene data.

  Parameters:
  - <a id="cutscenes.play.num" name="cutscenes.play.num"></a>**`num`** ([trx.cutscenes.Num](#cutscenes.Num)).

  Example:
  ```lua
  trx.cutscenes.play(28)
  ```

- <a id="cutscenes.is_played" name="cutscenes.is_played"></a>[lua]`trx.cutscenes.is_played(num)`  
  Whether a cutscene trigger naming this number has already been answered.

  Parameters:
  - <a id="cutscenes.is_played.num" name="cutscenes.is_played.num"></a>**`num`** ([trx.cutscenes.Num](#cutscenes.Num)).

  Returns: boolean. True once it has run, which is what keeps its trigger from firing again.

- <a id="cutscenes.set_played" name="cutscenes.set_played"></a>[lua]`trx.cutscenes.set_played(num, played)`  
  Marks a cutscene as played or unplayed. Marking one as played keeps its
  trigger from firing; unmarking one lets it run again.

  A trigger may name a number the game has no cutscene for - TR4 uses 32 to
  ask for a full-motion video - and the engine remembers those the same way,
  so [`trx.events.on_cutscene_trigger`](EVENTS.md#events.on_cutscene_trigger) hears about each of them once. This is what clears
  that memory, and it takes any number a trigger may carry, not only the ones
  [`trx.cutscenes.play`](#cutscenes.play) accepts.

  Parameters:
  - <a id="cutscenes.set_played.num" name="cutscenes.set_played.num"></a>**`num`** ([trx.cutscenes.Num](#cutscenes.Num)).
  - <a id="cutscenes.set_played.played" name="cutscenes.set_played.played"></a>**`played`** (boolean). Whether it counts as played.

  Example:
  ```lua
  trx.cutscenes.set_played(7, true)
  ```

- <a id="cutscenes.forget_played" name="cutscenes.forget_played"></a>[lua]`trx.cutscenes.forget_played()`  
  Forgets every cutscene, so all of them may run again.

- <a id="cutscenes.set_lara_return" name="cutscenes.set_lara_return"></a>[lua]`trx.cutscenes.set_lara_return(pos, [rot])`  
  Places Lara where the next cutscene to end leaves her. A cutscene stands
  her at its own origin while it plays and puts her back where it found her
  afterwards; this says to put her somewhere else instead, as the original
  game does for the scenes that carry her along.

  It holds for one cutscene, whether named before [`trx.cutscenes.play`](#cutscenes.play) or while the scene
  runs, and is forgotten once she has been placed.

  Parameters:
  - <a id="cutscenes.set_lara_return.pos" name="cutscenes.set_lara_return.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position.
  - <a id="cutscenes.set_lara_return.rot" name="cutscenes.set_lara_return.rot"></a>**`rot`** ([trx.math.Angle](MATH.md#math.Angle), optional). Facing angle. Defaults to `0`.

  Example:
  ```lua
  trx.events.on_cutscene_start(function(num)
    if num == 12 then
      trx.cutscenes.set_lara_return({ x = 38912, y = 2048, z = 51200 })
    end
  end)
  ```
