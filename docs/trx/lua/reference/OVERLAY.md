---
title: Overlay
order: 41
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: tools/lint/gen/lua_docs
  The public API is declared next to its implementation, in
  src/lua/trx/overlay.lua. Edit it there.
-->

## <a id="overlay" name="overlay"></a>Overlay module

What the engine draws over the game and no script owns: the pickups that
slide in, the assault course digits, and the lines of text the rest of the
engine asks for.

Only what a script has to answer for is here. The lines of text are the
engine's own, and a script neither reads nor writes them.

### Properties

- <a id="overlay.has_letterbox" name="overlay.has_letterbox"></a>**`trx.overlay.has_letterbox`** (boolean). Whether the cinematic bars take any of the screen. It stays true while they move, so a script can hold something back until they have gone. *(read-only)*
- <a id="overlay.letterbox" name="overlay.letterbox"></a>**`trx.overlay.letterbox`** (number). How deep the cinematic bars are asked to be, as a fraction of the screen height each, from `0` to `0.5`. Setting it slides the bars there; `0` takes them away. Flyby cameras and cutscenes set it too, and the last request wins. [`trx.ui.working_area`](UI.md#ui.working_area) follows the bars as they move.
- <a id="overlay.signals.letterbox" name="overlay.signals.letterbox"></a>**`trx.overlay.signals.letterbox`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when the cinematic bars take the screen and when they give it back. True while they are moving, so a script can hold something back until they have gone. *(read-only)*
- <a id="overlay.signals.health_bar_forced" name="overlay.signals.health_bar_forced"></a>**`trx.overlay.signals.health_bar_forced`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when something asks for Lara's health bar whatever else is on screen, which the inventory ring does while it shows a medipack. *(read-only)*

### Enums

- <a id="overlay.Arrow" name="overlay.Arrow"></a>[lua]`trx.overlay.Arrow`

    An arrow the interface can show.

    - `trx.overlay.Arrow.TOP_LEFT`  
        The top-left screen corner.
    - `trx.overlay.Arrow.TOP_RIGHT`  
        The top-right screen corner.
    - `trx.overlay.Arrow.BOTTOM_LEFT`  
        The bottom-left screen corner.
    - `trx.overlay.Arrow.BOTTOM_RIGHT`  
        The bottom-right screen corner.
    - `trx.overlay.Arrow.CAPTION_LEFT`  
        To the left of the caption.
    - `trx.overlay.Arrow.CAPTION_RIGHT`  
        To the right of the caption.

### Functions

- <a id="overlay.signals" name="overlay.signals"></a>[lua]`trx.overlay.signals`  
  What the overlay tells a script, for the parts of it a script draws.

- <a id="overlay.set_caption" name="overlay.set_caption"></a>[lua]`trx.overlay.set_caption([text], [count])`  
  Sets the caption at the bottom of the screen.

  The inventory ring uses it for the selected entry. The caption reduces the
  safe area. Passing no value removes it.

  The count stands above the caption, where the ring puts the count of an
  entry Lara carries more than one of, and shows while the ring is open.

  Parameters:
  - <a id="overlay.set_caption.text" name="overlay.set_caption.text"></a>**`text`** (string, optional). Text to show. Omit this parameter to remove the caption.
  - <a id="overlay.set_caption.count" name="overlay.set_caption.count"></a>**`count`** (integer, optional). The count to show with it. Omit it to show none.

- <a id="overlay.show_arrow" name="overlay.show_arrow"></a>[lua]`trx.overlay.show_arrow(arrow, shown)`  
  Shows or hides an interface arrow.

  The caption arrows stand beside the caption that [`trx.overlay.set_caption`](#overlay.set_caption)
  sets.

  Parameters:
  - <a id="overlay.show_arrow.arrow" name="overlay.show_arrow.arrow"></a>**`arrow`** ([trx.overlay.Arrow](#overlay.Arrow)). Which arrow to show.
  - <a id="overlay.show_arrow.shown" name="overlay.show_arrow.shown"></a>**`shown`** (boolean). Whether it is on screen.

- <a id="overlay.show_pickup" name="overlay.show_pickup"></a>[lua]`trx.overlay.show_pickup(object)`  
  Shows an object in the bottom-right corner of the screen, as the game shows
  a pickup Lara collects. It fires [`trx.events.on_show_pickup`](EVENTS.md#events.on_show_pickup). It also plays
  secret music for a secret pickup.

  It does not add the object to Lara's inventory. Use
  [`trx.inventory.Inventory:give`](INVENTORY.md#inventory.Inventory.give) for that.

  Parameters:
  - <a id="overlay.show_pickup.object" name="overlay.show_pickup.object"></a>**`object`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The object to show.

  Example:
  ```lua
  trx.overlay.show_pickup(trx.catalog.objects.KEY_ITEM_1)
  ```
