---
title: Sound
---

## Sound module

## Functions

- [lua]`TRX.Sound.IsAvailable(id)`  
    Returns `true` if the specified sound sample is available.
- [lua]`TRX.Sound.Stop(id)`  
    Stops the specified sound effect.
- [lua]`TRX.Sound.Play(id[, opts])`  
    Plays specified sound effect. `opts.pos` may be a `{ x=, y=, z= }` table for position.  
    Examples:
    - [lua]`TRX.Sound.Play(99)`  
      Plays the sound 99 (in TR1, this is an explosion, in TR2 this is a tiger's roar) at full volume.
    - [lua]`TRX.Sound.Play(99, { pos = { x = 100, y = 200, z = 50 } })`  
      Plays the same sound at world position (100,200,50), applying pan and volume accordingly.
- [lua]`TRX.Sound.StopAll()`  
    Stops all currently playing sound effects.
