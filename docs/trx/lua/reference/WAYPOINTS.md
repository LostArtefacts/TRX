---
title: Waypoints
order: 36
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/waypoints.lua. Edit it there.
-->

## <a id="waypoints" name="waypoints"></a>Waypoints module

Module for how far along a level's own progression Lara has got.

TR4 marks the points of a level with flip effects, and its guides read
them: Von Croy waits at a waypoint until Lara has reached it, says the
line that belongs to it, and only then moves on. A level uses the same
marks to tell a first visit from a return.

Nothing about a waypoint is positional. It counts progress, and it lasts
as long as the playthrough rather than the level, so it is saved with the
game.

### Properties

- <a id="waypoints.current" name="waypoints.current"></a>**`trx.waypoints.current`** ([trx.waypoints.Num](#waypoints.Num)). Where Lara has reached, or `nil` before she has reached anywhere. Setting
  it carries the furthest reached along with it where that is further on.
- <a id="waypoints.pad" name="waypoints.pad"></a>**`trx.waypoints.pad`** ([trx.waypoints.Num](#waypoints.Num)). The pad Lara crossed this frame, or `nil` on any frame she crossed none.
  It says where she is standing now rather than how far she has got, and
  it is meant to last the one frame: whoever sets it clears it again at the
  start of the next, which is `nil` here.

  Setting it carries [`trx.waypoints.current`](#waypoints.current) along with it, but leaves
  [`trx.waypoints.highest`](#waypoints.highest) alone.
- <a id="waypoints.highest" name="waypoints.highest"></a>**`trx.waypoints.highest`** ([trx.waypoints.Num](#waypoints.Num)). The furthest Lara has ever reached, or `nil` before she has reached
  anywhere. It never falls, so a level that lets her walk back can still
  tell how far she got. *(read-only)*

### Structures

- <a id="waypoints.Num" name="waypoints.Num"></a>[lua]`trx.waypoints.Num`

    A waypoint's number, as the flip effect that marks it names it. Counted from 0.
