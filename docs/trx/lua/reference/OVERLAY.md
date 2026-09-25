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

### Enums

- <a id="overlay.Arrow" name="overlay.Arrow"></a>[lua]`trx.overlay.Arrow`

    An arrow the interface can show.

    - `trx.overlay.Arrow.TOP_LEFT` = `0`  
        The top-left screen corner.
    - `trx.overlay.Arrow.TOP_RIGHT` = `1`  
        The top-right screen corner.
    - `trx.overlay.Arrow.BOTTOM_LEFT` = `2`  
        The bottom-left screen corner.
    - `trx.overlay.Arrow.BOTTOM_RIGHT` = `3`  
        The bottom-right screen corner.
    - `trx.overlay.Arrow.CAPTION_LEFT` = `4`  
        To the left of the caption.
    - `trx.overlay.Arrow.CAPTION_RIGHT` = `5`  
        To the right of the caption.

### Functions

- <a id="overlay.signals" name="overlay.signals"></a>[lua]`trx.overlay.signals`  
  What the overlay tells a script, for the parts of it a script draws.

- <a id="overlay.set_caption" name="overlay.set_caption"></a>[lua]`trx.overlay.set_caption([text])`  
  Sets the caption at the bottom of the screen.

  The inventory ring uses it for the selected entry. The passport uses it for the
  current page. The caption reduces the safe area. Passing no value removes it.

  Parameters:
  - <a id="overlay.set_caption.text" name="overlay.set_caption.text"></a>**`text`** (string, optional). Text to show. Omit this parameter to remove the caption.

- <a id="overlay.show_arrow" name="overlay.show_arrow"></a>[lua]`trx.overlay.show_arrow(arrow, shown)`  
  Shows or hides an interface arrow.

  The passport uses the caption arrows to show which way the book turns.

  Parameters:
  - <a id="overlay.show_arrow.arrow" name="overlay.show_arrow.arrow"></a>**`arrow`** ([trx.overlay.Arrow](#overlay.Arrow)). Which arrow to show.
  - <a id="overlay.show_arrow.shown" name="overlay.show_arrow.shown"></a>**`shown`** (boolean). Whether it is on screen.

- <a id="overlay.show_pickup" name="overlay.show_pickup"></a>[lua]`trx.overlay.show_pickup(object)`  
  Shows an object in the bottom-right corner of the screen, as the game shows a
  pickup Lara collects. It fires [`trx.events.on_show_pickup`](EVENTS.md#events.on_show_pickup). It also plays secret
  music for a secret pickup.

  It does not add the object to Lara's inventory. Use [`trx.inventory.Inventory:give`](INVENTORY.md#inventory.Inventory.give)
  for that.

  Parameters:
  - <a id="overlay.show_pickup.object" name="overlay.show_pickup.object"></a>**`object`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The object to show.

  Example:
  ```lua
  trx.overlay.show_pickup(trx.catalog.objects.KEY_ITEM_1)
  ```
