---
title: User interface
order: 19
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/ui.lua. Edit it there.
-->

## <a id="ui" name="ui"></a>User interface module

Module for drawing on top of the game.

Every function here is available only from a [`trx.events.on_ui_draw`](EVENTS.md#events.on_ui_draw) handler,
and raises anywhere else: the interface is built afresh each drawn frame, and
there is no scene to add to outside one.

A handler adds to the region the game is building, which it is told the name
of. Widgets land in the same stack as the health bars and the item names, so a
script cannot draw over them and the player's choice of where each element sits
still holds.

Widgets that hold other widgets take the body as a function rather than opening
and closing by hand, so a scene stays whole even where the body fails.

Sizes are in canvas units, not screen pixels. [`trx.ui.canvas`](#ui.canvas) reports the
canvas, and [`trx.ui.safe_area`](#ui.safe_area) the part of it that is free to draw in.

Text carries the same markup the rest of the game uses, and it is part of this
API: `\{small}` draws the rest of the line small, `\{arrow up}` draws an arrow,
and `\{button left}` draws the button the player has bound.

### Properties

- <a id="ui.canvas" name="ui.canvas"></a>**`trx.ui.canvas`** ([trx.ui.Area](#ui.Area)). The whole canvas. Widget sizes are in these units rather than in screen pixels, and the canvas is 640 by 480 for a 4:3 screen at the default text size. *(read-only)*
- <a id="ui.safe_area" name="ui.safe_area"></a>**`trx.ui.safe_area`** ([trx.ui.Area](#ui.Area)). The part of the canvas that is free to draw in: the canvas, less the margin kept at the edges, less what the game reserves at the top and the bottom for the bars and the text it puts there. *(read-only)*

### Enums

- <a id="ui.Orientation" name="ui.Orientation"></a>[lua]`trx.ui.Orientation`

    The direction a stack lays its children out in.

    - `trx.ui.Orientation.VERTICAL` = `0`  
        One below the next.
    - `trx.ui.Orientation.HORIZONTAL` = `1`  
        One beside the next.

- <a id="ui.HAlign" name="ui.HAlign"></a>[lua]`trx.ui.HAlign`

    Where a stack puts its children across its width.

    - `trx.ui.HAlign.LEFT` = `0`  
        Against the left edge.
    - `trx.ui.HAlign.CENTER` = `1`  
        In the middle.
    - `trx.ui.HAlign.RIGHT` = `2`  
        Against the right edge.
    - `trx.ui.HAlign.SPAN` = `3`  
        Stretched to the full width.
    - `trx.ui.HAlign.DISTRIBUTE` = `4`  
        Spread out, with the gaps taking the spare width.

- <a id="ui.VAlign" name="ui.VAlign"></a>[lua]`trx.ui.VAlign`

    Where a stack puts its children down its height.

    - `trx.ui.VAlign.TOP` = `0`  
        Against the top edge.
    - `trx.ui.VAlign.CENTER` = `1`  
        In the middle.
    - `trx.ui.VAlign.BOTTOM` = `2`  
        Against the bottom edge.
    - `trx.ui.VAlign.SPAN` = `3`  
        Stretched to the full height.
    - `trx.ui.VAlign.DISTRIBUTE` = `4`  
        Spread out, with the gaps taking the spare height.

- <a id="ui.Region" name="ui.Region"></a>[lua]`trx.ui.Region`

    One of the nine places the interface is built in. A handler is told which one is being built and adds to it, and everything asking for a place is laid out together there rather than over what else asked for it.

    The eight around the edge stack what they hold away from the edge they sit at. The middle is what the others leave, and is where a dialog goes.

    - `trx.ui.Region.TOP_LEFT` = `0`  
        The top left corner.
    - `trx.ui.Region.TOP_CENTER` = `1`  
        The top edge, in the middle.
    - `trx.ui.Region.TOP_RIGHT` = `2`  
        The top right corner.
    - `trx.ui.Region.LEFT` = `3`  
        The left edge, halfway down.
    - `trx.ui.Region.CENTER` = `4`  
        The middle of the screen, inside what the others leave.
    - `trx.ui.Region.RIGHT` = `5`  
        The right edge, halfway down.
    - `trx.ui.Region.BOTTOM_LEFT` = `6`  
        The bottom left corner.
    - `trx.ui.Region.BOTTOM_CENTER` = `7`  
        The bottom edge, in the middle.
    - `trx.ui.Region.BOTTOM_RIGHT` = `8`  
        The bottom right corner.

- <a id="ui.FrameStyle" name="ui.FrameStyle"></a>[lua]`trx.ui.FrameStyle`

    The look a frame draws around what it holds.

    - `trx.ui.FrameStyle.DIALOG_BACKGROUND` = `0`  
        The background a dialog sits on.
    - `trx.ui.FrameStyle.DIALOG_BACKGROUND_HEAVY` = `1`  
        The same background, drawn heavier.
    - `trx.ui.FrameStyle.DIALOG_HEADING` = `2`  
        The band a dialog puts its title in.
    - `trx.ui.FrameStyle.SELECTED_OPTION` = `3`  
        The highlight on the option the player is on.
    - `trx.ui.FrameStyle.OUTLINE_ONLY` = `4`  
        An outline, with nothing behind it.

- <a id="ui.BarType" name="ui.BarType"></a>[lua]`trx.ui.BarType`

    Which of the game's bars to draw, which decides its colors.

    - `trx.ui.BarType.LARA_HP` = `0`  
        Lara's health.
    - `trx.ui.BarType.LARA_HP_POISON` = `1`  
        Lara's health while she is poisoned.
    - `trx.ui.BarType.LARA_AIR` = `2`  
        Lara's air.
    - `trx.ui.BarType.LARA_STAMINA` = `3`  
        Lara's stamina.
    - `trx.ui.BarType.LARA_EXPOSURE` = `4`  
        Lara's exposure to the cold.
    - `trx.ui.BarType.ENEMY_HP` = `5`  
        An enemy's health.
    - `trx.ui.BarType.ALLY_HP` = `6`  
        An ally's health.
    - `trx.ui.BarType.PROGRESS` = `7`  
        A general progress bar.

### Structures

- <a id="ui.Area" name="ui.Area"></a>[lua]`trx.ui.Area`

    A rectangle on the canvas, in canvas units, counted from the top left.

    Properties:
    - <a id="ui.Area.height" name="ui.Area.height"></a>**`height`**: number. How tall it is.
    - <a id="ui.Area.width" name="ui.Area.width"></a>**`width`**: number. How wide it is.
    - <a id="ui.Area.x" name="ui.Area.x"></a>**`x`**: number. The left edge.
    - <a id="ui.Area.y" name="ui.Area.y"></a>**`y`**: number. The top edge.

### Functions

- <a id="ui.label" name="ui.label"></a>[lua]`trx.ui.label(text, [settings])`  
  Draws a line of text. It takes the player's font and text size, and the text markup applies.

  Parameters:
  - <a id="ui.label.text" name="ui.label.text"></a>**`text`** (string). What to draw.
  - <a id="ui.label.settings" name="ui.label.settings"></a>**`settings`** (table, optional). How to draw it.

    Keys:
    - <a id="ui.label.settings.scale" name="ui.label.settings.scale"></a>**`scale`** (number, optional). How much to multiply the text size by. `1.0` by default.
    - <a id="ui.label.settings.z" name="ui.label.settings.z"></a>**`z`** (integer, optional). What to draw in front of. Higher draws later. `0` by default.

- <a id="ui.measure" name="ui.measure"></a>[lua]`trx.ui.measure(text, [settings])`  
  Answers how much room a line of text takes, without drawing it. Unlike the rest of the module, this is available at any time.

  Parameters:
  - <a id="ui.measure.text" name="ui.measure.text"></a>**`text`** (string). What to measure.
  - <a id="ui.measure.settings" name="ui.measure.settings"></a>**`settings`** (table, optional). The settings [`trx.ui.label`](#ui.label) would draw it with.

    Keys:
    - <a id="ui.measure.settings.scale" name="ui.measure.settings.scale"></a>**`scale`** (number, optional). How much to multiply the text size by. `1.0` by default.

  Returns:
  - number. The width, in canvas units.
  - number. The height, in canvas units.

- <a id="ui.spacer" name="ui.spacer"></a>[lua]`trx.ui.spacer(w, h)`  
  Leaves a gap. It draws nothing and takes the room it is given.

  Parameters:
  - <a id="ui.spacer.w" name="ui.spacer.w"></a>**`w`** (number). The width, in canvas units.
  - <a id="ui.spacer.h" name="ui.spacer.h"></a>**`h`** (number). The height, in canvas units.

- <a id="ui.bar" name="ui.bar"></a>[lua]`trx.ui.bar(settings)`  
  Draws one of the game's own bars, in the player's chosen bar style.

  Parameters:
  - <a id="ui.bar.settings" name="ui.bar.settings"></a>**`settings`** (table). Which bar to draw and how full it is.

    Keys:
    - <a id="ui.bar.settings.type" name="ui.bar.settings.type"></a>**`type`** ([trx.ui.BarType](#ui.BarType), optional). Which bar it is. [`trx.ui.BarType.LARA_HP`](#ui.BarType) by default.
    - <a id="ui.bar.settings.value" name="ui.bar.settings.value"></a>**`value`** (integer, optional). How much is filled. `0` by default.
    - <a id="ui.bar.settings.max_value" name="ui.bar.settings.max_value"></a>**`max_value`** (integer, optional). What a full bar holds. `100` by default.
    - <a id="ui.bar.settings.w" name="ui.bar.settings.w"></a>**`w`** (integer, optional). The width, in canvas units. The game's own width by default.
    - <a id="ui.bar.settings.h" name="ui.bar.settings.h"></a>**`h`** (integer, optional). The height, in canvas units. The game's own height by default.

  Example:
  ```lua
  trx.ui.bar({
    type = trx.ui.BarType.PROGRESS,
    value = 40,
    max_value = 100,
  })
  ```

- <a id="ui.stack" name="ui.stack"></a>[lua]`trx.ui.stack(settings, body)`  
  Lays several widgets out in a row or a column.

  Parameters:
  - <a id="ui.stack.settings" name="ui.stack.settings"></a>**`settings`** (table). How to lay the children out.

    Keys:
    - <a id="ui.stack.settings.orientation" name="ui.stack.settings.orientation"></a>**`orientation`** ([trx.ui.Orientation](#ui.Orientation), optional). Which way the children run. [`trx.ui.Orientation.VERTICAL`](#ui.Orientation) by default.
    - <a id="ui.stack.settings.align" name="ui.stack.settings.align"></a>**`align`** (table, optional). Where the children sit within the stack.
    - <a id="ui.stack.settings.spacing" name="ui.stack.settings.spacing"></a>**`spacing`** (table, optional). The gap left between one child and the next.
  - <a id="ui.stack.body" name="ui.stack.body"></a>**`body`** (function). Draws the children.

  Example:
  ```lua
  trx.ui.stack({
    orientation = trx.ui.Orientation.HORIZONTAL,
    spacing = { h = 4 },
  }, function()
    trx.ui.label("00:12")
    trx.ui.label("\{small}elapsed")
  end)
  ```

- <a id="ui.anchor" name="ui.anchor"></a>[lua]`trx.ui.anchor(x, y, body)`  
  Puts what it holds at a spot in the room it is given. The spot is a ratio, so `0.5, 0.5` is the middle and `1.0, 0.0` is the top right.

  Parameters:
  - <a id="ui.anchor.x" name="ui.anchor.x"></a>**`x`** (number). How far across, from 0 to 1.
  - <a id="ui.anchor.y" name="ui.anchor.y"></a>**`y`** (number). How far down, from 0 to 1.
  - <a id="ui.anchor.body" name="ui.anchor.body"></a>**`body`** (function). Draws what it holds.

- <a id="ui.pad" name="ui.pad"></a>[lua]`trx.ui.pad(settings, body)`  
  Keeps a border clear around what it holds.

  Parameters:
  - <a id="ui.pad.settings" name="ui.pad.settings"></a>**`settings`** (table). How much to keep clear, in canvas units.

    Keys:
    - <a id="ui.pad.settings.x" name="ui.pad.settings.x"></a>**`x`** (number, optional). How much to keep clear at the left and the right. `0` by default.
    - <a id="ui.pad.settings.y" name="ui.pad.settings.y"></a>**`y`** (number, optional). How much to keep clear at the top and the bottom. `0` by default.
    - <a id="ui.pad.settings.l" name="ui.pad.settings.l"></a>**`l`** (number, optional). The left side on its own, which wins over the pair.
    - <a id="ui.pad.settings.r" name="ui.pad.settings.r"></a>**`r`** (number, optional). The right side on its own, which wins over the pair.
    - <a id="ui.pad.settings.t" name="ui.pad.settings.t"></a>**`t`** (number, optional). The top on its own, which wins over the pair.
    - <a id="ui.pad.settings.b" name="ui.pad.settings.b"></a>**`b`** (number, optional). The bottom on its own, which wins over the pair.
  - <a id="ui.pad.body" name="ui.pad.body"></a>**`body`** (function). Draws what it holds.

- <a id="ui.hide" name="ui.hide"></a>[lua]`trx.ui.hide(hidden, body)`  
  Draws what it holds, or not. What it holds takes its room either way, so a widget that comes and goes does not move the widgets beside it.

  Parameters:
  - <a id="ui.hide.hidden" name="ui.hide.hidden"></a>**`hidden`** (boolean). Whether to leave it undrawn.
  - <a id="ui.hide.body" name="ui.hide.body"></a>**`body`** (function). Draws what it holds.

- <a id="ui.resize" name="ui.resize"></a>[lua]`trx.ui.resize(settings, body)`  
  Gives what it holds a size of its own.

  Parameters:
  - <a id="ui.resize.settings" name="ui.resize.settings"></a>**`settings`** (table). The size to give it, in canvas units.

    Keys:
    - <a id="ui.resize.settings.w" name="ui.resize.settings.w"></a>**`w`** (number, optional). The width. Its own width by default; `0` hides it but keeps its room.
    - <a id="ui.resize.settings.h" name="ui.resize.settings.h"></a>**`h`** (number, optional). The height. Its own height by default; `0` hides it but keeps its room.
    - <a id="ui.resize.settings.align_h" name="ui.resize.settings.align_h"></a>**`align_h`** (number, optional). Where it sits across the width, from 0 to 1. `0` by default.
    - <a id="ui.resize.settings.align_v" name="ui.resize.settings.align_v"></a>**`align_v`** (number, optional). Where it sits down the height, from 0 to 1. `0` by default.
  - <a id="ui.resize.body" name="ui.resize.body"></a>**`body`** (function). Draws what it holds.

- <a id="ui.frame" name="ui.frame"></a>[lua]`trx.ui.frame(style, body)`  
  Draws one of the game's frames behind what it holds.

  Parameters:
  - <a id="ui.frame.style" name="ui.frame.style"></a>**`style`** ([trx.ui.FrameStyle](#ui.FrameStyle)). Which frame to draw.
  - <a id="ui.frame.body" name="ui.frame.body"></a>**`body`** (function). Draws what it holds.

- <a id="ui.offset" name="ui.offset"></a>[lua]`trx.ui.offset(x, y, body)`  
  Moves what it holds. The room it takes does not move with it, so the widgets beside it stay where they are.

  Parameters:
  - <a id="ui.offset.x" name="ui.offset.x"></a>**`x`** (number). How far to move it across, in canvas units.
  - <a id="ui.offset.y" name="ui.offset.y"></a>**`y`** (number). How far to move it down, in canvas units.
  - <a id="ui.offset.body" name="ui.offset.body"></a>**`body`** (function). Draws what it holds.

- <a id="ui.span" name="ui.span"></a>[lua]`trx.ui.span(body)`  
  Draws what it holds one on top of the next, every one as big as the biggest.

  Parameters:
  - <a id="ui.span.body" name="ui.span.body"></a>**`body`** (function). Draws what it holds.
