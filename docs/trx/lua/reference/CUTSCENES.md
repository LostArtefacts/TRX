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
- <a id="cutscenes.count" name="cutscenes.count"></a>**`trx.cutscenes.count`** (integer). How many cutscenes this game can play. `0` where it has none, which is every game but TR4 and a TR4 install with no `cutseq.pak` beside its levels. *(read-only)*
- <a id="cutscenes.actor_count" name="cutscenes.actor_count"></a>**`trx.cutscenes.actor_count`** (integer). How many actors the running cutscene has, or `0` if none is running. *(read-only)*
- <a id="cutscenes.fov" name="cutscenes.fov"></a>**`trx.cutscenes.fov`** ([trx.math.Angle](MATH.md#math.Angle)). Field of view a cutscene plays at. TR4 uses 11488, against 14560 for ordinary play.
- <a id="cutscenes.letterbox" name="cutscenes.letterbox"></a>**`trx.cutscenes.letterbox`** (number). Depth of each cinematic bar, as a fraction of the screen height. `0` removes them.

### Structures

- <a id="cutscenes.Num" name="cutscenes.Num"></a>[lua]`trx.cutscenes.Num`

    Cutscene number, as a cutscene trigger names it. Counted from 0.

- <a id="cutscenes.FrameNum" name="cutscenes.FrameNum"></a>[lua]`trx.cutscenes.FrameNum`

    A frame's number within the cutscene it belongs to. Counted from 0.

- <a id="cutscenes.ActorNum" name="cutscenes.ActorNum"></a>[lua]`trx.cutscenes.ActorNum`

    Which of a cutscene's actors. Actor `0` is Lara, who is posed rather than
    drawn as an actor; the cast a scene brings with it starts at `1`. Counted from 0.

- <a id="cutscenes.NodeNum" name="cutscenes.NodeNum"></a>[lua]`trx.cutscenes.NodeNum`

    Which of an actor's meshes, the root being the first. Counted from 0.

### Functions

- <a id="cutscenes.play" name="cutscenes.play"></a>[lua]`trx.cutscenes.play(num, [fade])`  
  Plays a cutscene, fading the scene out first. Does nothing if one is already playing or the game has no cutscene data.

  Parameters:
  - <a id="cutscenes.play.num" name="cutscenes.play.num"></a>**`num`** ([trx.cutscenes.Num](#cutscenes.Num)).
  - <a id="cutscenes.play.fade" name="cutscenes.play.fade"></a>**`fade`** (boolean, optional). Whether to fade the scene out before the first frame. Defaults to true. A cutscene that opens a level passes false: the original game holds the screen black rather than showing the level for a moment first, and the scene's own fade in follows either way.

  Example:
  ```lua
  trx.cutscenes.play(28)
  ```

- <a id="cutscenes.set_actor_visible" name="cutscenes.set_actor_visible"></a>[lua]`trx.cutscenes.set_actor_visible(actor, visible)`  
  Whether an actor is drawn. A scene brings its whole cast on from its first
  frame, so an actor who is only due later is hidden until then, as the
  original game hides one.

  It lasts as long as the cutscene, and every actor starts out visible.

  Parameters:
  - <a id="cutscenes.set_actor_visible.actor" name="cutscenes.set_actor_visible.actor"></a>**`actor`** ([trx.cutscenes.ActorNum](#cutscenes.ActorNum)).
  - <a id="cutscenes.set_actor_visible.visible" name="cutscenes.set_actor_visible.visible"></a>**`visible`** (boolean). Whether the actor is drawn.

  Example:
  ```lua
  trx.events.on_cutscene_start(function(num)
    if num == 9 then
      trx.cutscenes.set_actor_visible(3, false)
    end
  end)
  ```

- <a id="cutscenes.set_node_mesh" name="cutscenes.set_node_mesh"></a>[lua]`trx.cutscenes.set_node_mesh(actor, node, object, [mesh_num])`  
  Draws another object's mesh in place of the one an actor's node carries.
  This is how a talking head goes on a body: the speech-head objects hold a
  mouth in each shape, and swapping between them while a line plays is what
  the original game animates speech with.

  Raises if this level does not carry the object.

  Parameters:
  - <a id="cutscenes.set_node_mesh.actor" name="cutscenes.set_node_mesh.actor"></a>**`actor`** ([trx.cutscenes.ActorNum](#cutscenes.ActorNum)).
  - <a id="cutscenes.set_node_mesh.node" name="cutscenes.set_node_mesh.node"></a>**`node`** ([trx.cutscenes.NodeNum](#cutscenes.NodeNum)).
  - <a id="cutscenes.set_node_mesh.object" name="cutscenes.set_node_mesh.object"></a>**`object`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The object to take a mesh from.
  - <a id="cutscenes.set_node_mesh.mesh_num" name="cutscenes.set_node_mesh.mesh_num"></a>**`mesh_num`** (integer, optional). Which of that object's meshes. Defaults to `0`.

  Example:
  ```lua
  trx.cutscenes.set_node_mesh(1, 21, trx.catalog.objects.actor_1_speech_head_1)
  ```

- <a id="cutscenes.clear_node_mesh" name="cutscenes.clear_node_mesh"></a>[lua]`trx.cutscenes.clear_node_mesh(actor, node)`  
  Takes the override back off, leaving the mesh the actor's own object gives that node.

  Parameters:
  - <a id="cutscenes.clear_node_mesh.actor" name="cutscenes.clear_node_mesh.actor"></a>**`actor`** ([trx.cutscenes.ActorNum](#cutscenes.ActorNum)).
  - <a id="cutscenes.clear_node_mesh.node" name="cutscenes.clear_node_mesh.node"></a>**`node`** ([trx.cutscenes.NodeNum](#cutscenes.NodeNum)).

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
