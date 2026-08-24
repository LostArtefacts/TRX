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

- <a id="ui.Widget" name="ui.Widget"></a>[lua]`trx.ui.Widget`

    A reusable UI element drawn over the game.

    A widget holds its own state. Give it signals instead of fixed values, then
    register those signals with [`wakes_on`](#ui.Widget.wakes_on). The widget remeasures
    only when a registered signal changes.

    Register every signal that the widget reads. Otherwise the widget can keep a
    stale cached size.

    Methods:

    - <a id="ui.Widget.is_shown" name="ui.Widget.is_shown"></a>[lua]`widget:is_shown()`  
      Returns whether the widget participates in layout.

      A widget that is not shown keeps no room and leaves no gap. A hidden widget
      keeps its room but draws nothing.

      Returns: boolean. Whether it draws.

    - <a id="ui.Widget.measure" name="ui.Widget.measure"></a>[lua]`widget:measure()`  
      How much room the widget wants.

      Returns:
      - number. The width, in canvas units.
      - number. The height, in canvas units.

    - <a id="ui.Widget.paint" name="ui.Widget.paint"></a>[lua]`widget:paint(x, y, w, h)`  
      Draws the widget in an assigned box.

      [`trx.ui.regions.place`](#ui.regions.place) calls this automatically. Custom layout code can call it
      during [`trx.events.on_ui_paint`](EVENTS.md#events.on_ui_paint).

      Parameters:
      - <a id="ui.Widget.paint.x" name="ui.Widget.paint.x"></a>**`x`** (number). The left edge.
      - <a id="ui.Widget.paint.y" name="ui.Widget.paint.y"></a>**`y`** (number). The top edge.
      - <a id="ui.Widget.paint.w" name="ui.Widget.paint.w"></a>**`w`** (number). The width it was given.
      - <a id="ui.Widget.paint.h" name="ui.Widget.paint.h"></a>**`h`** (number). The height it was given.

    - <a id="ui.Widget.release" name="ui.Widget.release"></a>[lua]`widget:release()`  
      Detaches the widget and its children from registered signals.

      Signals keep references to their listeners. Release temporary widgets when they
      are no longer needed. Remove a placed widget from its region before releasing
      it.

      Returns: boolean. Whether it was still listening to anything.

    - <a id="ui.Widget.wake" name="ui.Widget.wake"></a>[lua]`widget:wake()`  
      Invalidates the widget's cached size manually.

      Returns: [trx.ui.Widget](#ui.Widget). The same widget.

    - <a id="ui.Widget.wakes_on" name="ui.Widget.wakes_on"></a>[lua]`widget:wakes_on(...)`  
      Registers the signals that invalidate the widget's cached size.

      When one of these signals changes, the widget and its parents are measured
      again on the next layout pass.

      Parameters:
      - <a id="ui.Widget.wakes_on...." name="ui.Widget.wakes_on...."></a>**`...`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). The signals the widget reads.

      Returns: [trx.ui.Widget](#ui.Widget). The same widget, for method chaining.

### Functions

- <a id="ui.primitive" name="ui.primitive"></a>[lua]`trx.ui.primitive`  
  Low-level drawing calls and layout reservations.

  Use [`trx.ui.widgets`](#ui.widgets) for normal UI. Use these primitives only when building a
  custom widget. Primitive drawing does not affect region layout unless code
  reserves space first.

  Drawing calls are available only during [`trx.events.on_ui_paint`](EVENTS.md#events.on_ui_paint). They report
  an error at any other time.

- <a id="ui.widgets" name="ui.widgets"></a>[lua]`trx.ui.widgets`  
  The widgets a script builds its screen from.

  A widget is created once and kept. Give it signals instead of fixed values, then
  register those signals with [`trx.ui.Widget:wakes_on`](#ui.Widget.wakes_on).

  Put a widget on screen with [`trx.ui.regions.place`](#ui.regions.place).

- <a id="ui.regions" name="ui.regions"></a>[lua]`trx.ui.regions`  
  Places script widgets on the screen.

  The screen has nine regions. Engine UI uses those regions for bars, overlay
  text, inventory-ring hints, and dialogs. A widget placed in a region stacks
  after the engine UI in that region.

  Place a widget once when the script loads. Use signals when the widget must
  change later.

- <a id="ui.primitive.reserve" name="ui.primitive.reserve"></a>[lua]`trx.ui.primitive.reserve(region, w, h)`  
  Reserves space in a region and returns a slot for it.

  The reservation is stacked with the engine UI in that region. Reserve space
  during [`trx.events.on_ui_draw`](EVENTS.md#events.on_ui_draw), then read the assigned box during
  [`trx.events.on_ui_paint`](EVENTS.md#events.on_ui_paint).

  A slot is valid only for the scene that created it.

  Parameters:
  - <a id="ui.primitive.reserve.region" name="ui.primitive.reserve.region"></a>**`region`** ([trx.ui.Region](#ui.Region)). Which region to keep room in.
  - <a id="ui.primitive.reserve.w" name="ui.primitive.reserve.w"></a>**`w`** (number). How wide, in canvas units.
  - <a id="ui.primitive.reserve.h" name="ui.primitive.reserve.h"></a>**`h`** (number). How tall, in canvas units.

  Returns: integer. The slot.

- <a id="ui.primitive.slot_box" name="ui.primitive.slot_box"></a>[lua]`trx.ui.primitive.slot_box(slot)`  
  Returns the box assigned to a reservation by the last layout.

  Parameters:
  - <a id="ui.primitive.slot_box.slot" name="ui.primitive.slot_box.slot"></a>**`slot`** (integer). The reservation slot.

  Returns:
  - number. The left edge, or `nil` when the slot is no longer valid.
  - number. The top edge.
  - number. The width.
  - number. The height.

- <a id="ui.primitive.measure_text" name="ui.primitive.measure_text"></a>[lua]`trx.ui.primitive.measure_text(text, [scale])`  
  Measures one line of text. Available at any time.

  Parameters:
  - <a id="ui.primitive.measure_text.text" name="ui.primitive.measure_text.text"></a>**`text`** (string). What to measure.
  - <a id="ui.primitive.measure_text.scale" name="ui.primitive.measure_text.scale"></a>**`scale`** (number, optional). Multiplies the text size. `1.0` by default.

  Returns:
  - number. The width, in canvas units.
  - number. The height, in canvas units.

- <a id="ui.primitive.text" name="ui.primitive.text"></a>[lua]`trx.ui.primitive.text(text, x, y, [scale], [z])`  
  Draws one line of text on the canvas.

  Parameters:
  - <a id="ui.primitive.text.text" name="ui.primitive.text.text"></a>**`text`** (string). What to draw.
  - <a id="ui.primitive.text.x" name="ui.primitive.text.x"></a>**`x`** (number). The left edge.
  - <a id="ui.primitive.text.y" name="ui.primitive.text.y"></a>**`y`** (number). The top edge.
  - <a id="ui.primitive.text.scale" name="ui.primitive.text.scale"></a>**`scale`** (number, optional). Multiplies the text size.
  - <a id="ui.primitive.text.z" name="ui.primitive.text.z"></a>**`z`** (integer, optional). The draw order.

- <a id="ui.primitive.to_screen" name="ui.primitive.to_screen"></a>[lua]`trx.ui.primitive.to_screen(length)`  
  Converts a canvas length to screen pixels.

  The canvas is a fixed 640x480 grid, and the screen size depends on the player
  settings and window. Use this with [`to_canvas`](#ui.primitive.to_canvas) when geometry
  must align to whole screen pixels, such as an even border.

  Parameters:
  - <a id="ui.primitive.to_screen.length" name="ui.primitive.to_screen.length"></a>**`length`** (number). A canvas length.

  Returns: number. The same length in screen pixels.

- <a id="ui.primitive.to_canvas" name="ui.primitive.to_canvas"></a>[lua]`trx.ui.primitive.to_canvas(pixels)`  
  Converts a screen-pixel length to canvas units.

  Use this with [`to_screen`](#ui.primitive.to_screen) when geometry must align to whole
  screen pixels.

  Parameters:
  - <a id="ui.primitive.to_canvas.pixels" name="ui.primitive.to_canvas.pixels"></a>**`pixels`** (number). A length in screen pixels.

  Returns: number. The same length in canvas units.

- <a id="ui.primitive.quad" name="ui.primitive.quad"></a>[lua]`trx.ui.primitive.quad(x, y, z, w, h, color)`  
  Draws a rectangle of one color.

  Parameters:
  - <a id="ui.primitive.quad.x" name="ui.primitive.quad.x"></a>**`x`** (number). The left edge.
  - <a id="ui.primitive.quad.y" name="ui.primitive.quad.y"></a>**`y`** (number). The top edge.
  - <a id="ui.primitive.quad.z" name="ui.primitive.quad.z"></a>**`z`** (integer). The draw order.
  - <a id="ui.primitive.quad.w" name="ui.primitive.quad.w"></a>**`w`** (number). The width.
  - <a id="ui.primitive.quad.h" name="ui.primitive.quad.h"></a>**`h`** (number). The height.
  - <a id="ui.primitive.quad.color" name="ui.primitive.quad.color"></a>**`color`** ([trx.math.Color](MATH.md#math.Color)). What color to fill it with.

- <a id="ui.primitive.gradient_quad" name="ui.primitive.gradient_quad"></a>[lua]`trx.ui.primitive.gradient_quad(x, y, z, w, h, tl, tr, bl, br)`  
  Draws a rectangle whose corners each carry a color.

  Parameters:
  - <a id="ui.primitive.gradient_quad.x" name="ui.primitive.gradient_quad.x"></a>**`x`** (number). The left edge.
  - <a id="ui.primitive.gradient_quad.y" name="ui.primitive.gradient_quad.y"></a>**`y`** (number). The top edge.
  - <a id="ui.primitive.gradient_quad.z" name="ui.primitive.gradient_quad.z"></a>**`z`** (integer). What to draw in front of.
  - <a id="ui.primitive.gradient_quad.w" name="ui.primitive.gradient_quad.w"></a>**`w`** (number). The width.
  - <a id="ui.primitive.gradient_quad.h" name="ui.primitive.gradient_quad.h"></a>**`h`** (number). The height.
  - <a id="ui.primitive.gradient_quad.tl" name="ui.primitive.gradient_quad.tl"></a>**`tl`** ([trx.math.Color](MATH.md#math.Color)). The top-left color.
  - <a id="ui.primitive.gradient_quad.tr" name="ui.primitive.gradient_quad.tr"></a>**`tr`** ([trx.math.Color](MATH.md#math.Color)). The top-right color.
  - <a id="ui.primitive.gradient_quad.bl" name="ui.primitive.gradient_quad.bl"></a>**`bl`** ([trx.math.Color](MATH.md#math.Color)). The bottom-left color.
  - <a id="ui.primitive.gradient_quad.br" name="ui.primitive.gradient_quad.br"></a>**`br`** ([trx.math.Color](MATH.md#math.Color)). The bottom-right color.

- <a id="ui.widgets.Label" name="ui.widgets.Label"></a>[lua]`trx.ui.widgets.Label(settings)`  
  A line of text. Use a signal for text that changes.

  Parameters:
  - <a id="ui.widgets.Label.settings" name="ui.widgets.Label.settings"></a>**`settings`** (table). The label settings.

    Keys:
    - <a id="ui.widgets.Label.settings.text" name="ui.widgets.Label.settings.text"></a>**`text`** (any). The text, or a signal carrying it.
    - <a id="ui.widgets.Label.settings.scale" name="ui.widgets.Label.settings.scale"></a>**`scale`** (number, optional). Multiplies the text size. `1.0` by default.
    - <a id="ui.widgets.Label.settings.shown" name="ui.widgets.Label.settings.shown"></a>**`shown`** (any, optional). Whether the label is shown, or a signal that holds that value.

  Returns: [trx.ui.Widget](#ui.Widget). The label.

- <a id="ui.widgets.Bar" name="ui.widgets.Bar"></a>[lua]`trx.ui.widgets.Bar(settings)`  
  One of the game's bars, drawn with the player's bar settings.

  The bar uses the same theme, border, and fill bands as the engine UI. Use a
  signal for a fill value that changes.

  Parameters:
  - <a id="ui.widgets.Bar.settings" name="ui.widgets.Bar.settings"></a>**`settings`** (table). The bar settings.

    Keys:
    - <a id="ui.widgets.Bar.settings.type" name="ui.widgets.Bar.settings.type"></a>**`type`** ([trx.ui.BarType](#ui.BarType)). The bar theme to use.
    - <a id="ui.widgets.Bar.settings.value" name="ui.widgets.Bar.settings.value"></a>**`value`** (any). The fill amount from 0 to 1, or a signal that holds it.
    - <a id="ui.widgets.Bar.settings.w" name="ui.widgets.Bar.settings.w"></a>**`w`** (number, optional). The width, in canvas units. The game's own by default.
    - <a id="ui.widgets.Bar.settings.h" name="ui.widgets.Bar.settings.h"></a>**`h`** (number, optional). The height, in canvas units. The game's own by default.
    - <a id="ui.widgets.Bar.settings.shown" name="ui.widgets.Bar.settings.shown"></a>**`shown`** (any, optional). Whether the bar is shown, or a signal that holds that value.

  Returns: [trx.ui.Widget](#ui.Widget). The bar.

- <a id="ui.widgets.Resize" name="ui.widgets.Resize"></a>[lua]`trx.ui.widgets.Resize(settings)`  
  Gives a child widget an explicit size.

  Use h_bars when a widget must match the height of the game's bars after the
  player's bar scale is applied.

  Parameters:
  - <a id="ui.widgets.Resize.settings" name="ui.widgets.Resize.settings"></a>**`settings`** (table). The resize settings.

    Keys:
    - <a id="ui.widgets.Resize.settings.child" name="ui.widgets.Resize.settings.child"></a>**`child`** ([trx.ui.Widget](#ui.Widget)). The child widget.
    - <a id="ui.widgets.Resize.settings.w" name="ui.widgets.Resize.settings.w"></a>**`w`** (number, optional). The width, in canvas units. Its own by default.
    - <a id="ui.widgets.Resize.settings.h" name="ui.widgets.Resize.settings.h"></a>**`h`** (number, optional). The height, in canvas units. Its own by default.
    - <a id="ui.widgets.Resize.settings.h_bars" name="ui.widgets.Resize.settings.h_bars"></a>**`h_bars`** (number, optional). The height in bar heights. This overrides the plain height.
    - <a id="ui.widgets.Resize.settings.shown" name="ui.widgets.Resize.settings.shown"></a>**`shown`** (any, optional). Whether the resized widget is shown, or a signal that holds that value.

  Returns: [trx.ui.Widget](#ui.Widget). The resized widget.

- <a id="ui.widgets.Row" name="ui.widgets.Row"></a>[lua]`trx.ui.widgets.Row(settings)`  
  A widget with a left and right arrow beside a child widget.

  Unlit arrows stay hidden but keep their room, so the child widget does not move
  when arrows appear or disappear.

  Parameters:
  - <a id="ui.widgets.Row.settings" name="ui.widgets.Row.settings"></a>**`settings`** (table). The row settings.

    Keys:
    - <a id="ui.widgets.Row.settings.child" name="ui.widgets.Row.settings.child"></a>**`child`** ([trx.ui.Widget](#ui.Widget)). The child widget placed between the arrows.
    - <a id="ui.widgets.Row.settings.left" name="ui.widgets.Row.settings.left"></a>**`left`** (any). Whether the left arrow is lit, or a signal that holds that value.
    - <a id="ui.widgets.Row.settings.right" name="ui.widgets.Row.settings.right"></a>**`right`** (any). Whether the right arrow is lit, or a signal that holds that value.
    - <a id="ui.widgets.Row.settings.spacing" name="ui.widgets.Row.settings.spacing"></a>**`spacing`** (number, optional). The gap between each arrow and the child widget. 15 by default.
    - <a id="ui.widgets.Row.settings.shown" name="ui.widgets.Row.settings.shown"></a>**`shown`** (any, optional). Whether the row is shown, or a signal that holds that value.

  Returns: [trx.ui.Widget](#ui.Widget). The row.

- <a id="ui.widgets.Stack" name="ui.widgets.Stack"></a>[lua]`trx.ui.widgets.Stack(settings)`  
  Lays widgets out one after another.

  Widgets that are not shown take no room and leave no gap.

  Parameters:
  - <a id="ui.widgets.Stack.settings" name="ui.widgets.Stack.settings"></a>**`settings`** (table). The stack settings.

    Keys:
    - <a id="ui.widgets.Stack.settings.children" name="ui.widgets.Stack.settings.children"></a>**`children`** (a list of table). The widgets, in the order they are laid out.
    - <a id="ui.widgets.Stack.settings.orientation" name="ui.widgets.Stack.settings.orientation"></a>**`orientation`** ([trx.ui.Orientation](#ui.Orientation), optional). The layout direction. Vertical by default.
    - <a id="ui.widgets.Stack.settings.spacing" name="ui.widgets.Stack.settings.spacing"></a>**`spacing`** (number, optional). The gap between one and the next. `0` by default.
    - <a id="ui.widgets.Stack.settings.align" name="ui.widgets.Stack.settings.align"></a>**`align`** ([trx.ui.HAlign](#ui.HAlign), optional). Where a narrower child sits in a vertical stack.
    - <a id="ui.widgets.Stack.settings.v_align" name="ui.widgets.Stack.settings.v_align"></a>**`v_align`** ([trx.ui.VAlign](#ui.VAlign), optional). Where a shorter child sits in a horizontal stack.
    - <a id="ui.widgets.Stack.settings.shown" name="ui.widgets.Stack.settings.shown"></a>**`shown`** (any, optional). Whether the stack is shown, or a signal that holds that value.

  Returns: [trx.ui.Widget](#ui.Widget). The stack.

- <a id="ui.regions.place" name="ui.regions.place"></a>[lua]`trx.ui.regions.place(region, widget)`  
  Places a widget in a region.

  If the region argument is a signal, the widget moves when the signal changes.

  Parameters:
  - <a id="ui.regions.place.region" name="ui.regions.place.region"></a>**`region`** (any). The target region, or a signal that holds one.
  - <a id="ui.regions.place.widget" name="ui.regions.place.widget"></a>**`widget`** ([trx.ui.Widget](#ui.Widget)). The widget to place.

  Example:
  ```lua
  trx.ui.regions.place(trx.ui.Region.TOP_LEFT, health_bar)
  ```

- <a id="ui.regions.remove" name="ui.regions.remove"></a>[lua]`trx.ui.regions.remove(widget)`  
  Removes a widget from its region.

  Use this for temporary widgets. Widgets owned by a level script are removed
  when the level ends. Call [`trx.ui.Widget:release`](#ui.Widget.release) separately to detach their
  signal listeners.

  Parameters:
  - <a id="ui.regions.remove.widget" name="ui.regions.remove.widget"></a>**`widget`** ([trx.ui.Widget](#ui.Widget)). The widget to remove.

  Returns: boolean. Whether the widget was in a region.

- <a id="ui.regions.fallback" name="ui.regions.fallback"></a>[lua]`trx.ui.regions.fallback(region, widget)`  
  Sets the widget to draw when a region has no visible content.

  A region with only non-shown widgets draws nothing. A fallback can reserve that
  empty place instead, for example the corner arrows shown when a bar is off
  screen. Each region has at most one fallback.

  Parameters:
  - <a id="ui.regions.fallback.region" name="ui.regions.fallback.region"></a>**`region`** ([trx.ui.Region](#ui.Region)). The target region.
  - <a id="ui.regions.fallback.widget" name="ui.regions.fallback.widget"></a>**`widget`** ([trx.ui.Widget](#ui.Widget)). The fallback widget.
