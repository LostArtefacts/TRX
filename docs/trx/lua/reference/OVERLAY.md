---
title: Overlay
order: 40
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/overlay.lua. Edit it there.
-->

## <a id="overlay" name="overlay"></a>Overlay module

What the engine draws over the game and no script owns: the pickups that slide in, the assault course digits, and the lines of text the rest of the engine asks for.

Only what a script has to answer for is here. The lines of text are the engine's own, and a script neither reads nor writes them.

### Properties

- <a id="overlay.has_letterbox" name="overlay.has_letterbox"></a>**`trx.overlay.has_letterbox`** (boolean). Whether the cinematic bars take any of the screen. It stays true while they
  move, so a script can hold something back until they have gone. *(read-only)*
- <a id="overlay.signals.letterbox" name="overlay.signals.letterbox"></a>**`trx.overlay.signals.letterbox`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when the cinematic bars take the screen and when they give it back. True
  while they are moving, so a script can hold something back until they have gone. *(read-only)*
- <a id="overlay.signals.health_bar_forced" name="overlay.signals.health_bar_forced"></a>**`trx.overlay.signals.health_bar_forced`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when something asks for Lara's health bar whatever else is on screen, which the inventory ring does while it shows a medipack. *(read-only)*

### Functions

- <a id="overlay.signals" name="overlay.signals"></a>[lua]`trx.overlay.signals`  
  What the overlay tells a script, for the parts of it a script draws.
