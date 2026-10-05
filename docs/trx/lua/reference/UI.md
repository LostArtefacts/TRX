---
title: User interface
order: 20
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: tools/lint/gen/lua_docs
  The public API is declared next to its implementation, in
  src/lua/trx/ui.lua. Edit it there.
-->

## <a id="ui" name="ui"></a>User interface module

Module for drawing on top of the game.

Every function here is available only from a [`trx.events.on_ui_draw`](EVENTS.md#events.on_ui_draw)
handler, and raises anywhere else: the interface is built afresh each drawn
frame, and there is no scene to add to outside one.

A handler adds to the region the game is building, which it is told the name
of. Widgets land in the same stack as the health bars and the item names, so
a script cannot draw over them and the player's choice of where each element
sits still holds.

Widgets that hold other widgets take the body as a function rather than
opening and closing by hand, so a scene stays whole even where the body
fails.

Sizes are in canvas units, not screen pixels. [`trx.ui.canvas`](#ui.canvas) reports the
canvas, and [`trx.ui.safe_area`](#ui.safe_area) the part of it that is free to draw in.

Text carries the same markup the rest of the game uses, and it is part of
this API: `\{small}` draws the rest of the line small, `\{arrow up}` draws an
arrow, and `\{button left}` draws the button the player has bound.

### Properties

- <a id="ui.canvas" name="ui.canvas"></a>**`trx.ui.canvas`** ([trx.ui.Area](#ui.Area)). The whole canvas. Widget sizes are in these units rather than in screen pixels, and the canvas is 640 by 480 for a 4:3 screen at the default text size. *(read-only)*
- <a id="ui.clipboard" name="ui.clipboard"></a>**`trx.ui.clipboard`** (string). What the system clipboard holds. Reads as an empty string where it holds nothing, and raises on assignment where the platform refuses the text.
  A script-drawn text field uses this value to paste and copy text.
- <a id="ui.text_scale" name="ui.text_scale"></a>**`trx.ui.text_scale`** (number). The scale applied to text and its boxes. The value depends on the player's text size and the screen. It is not the `ui.text_scale` setting alone. *(read-only)*
- <a id="ui.safe_area" name="ui.safe_area"></a>**`trx.ui.safe_area`** ([trx.ui.Area](#ui.Area)). The part of the canvas that is free to draw in: the canvas, less the margin kept at the edges, less what the game reserves at the top and the bottom for the bars and the text it puts there. *(read-only)*

### Enums

- <a id="ui.Orientation" name="ui.Orientation"></a>[lua]`trx.ui.Orientation`

    The direction a stack lays its children out in.

    - `trx.ui.Orientation.VERTICAL`  
        One below the next.
    - `trx.ui.Orientation.HORIZONTAL`  
        One beside the next.

- <a id="ui.HAlign" name="ui.HAlign"></a>[lua]`trx.ui.HAlign`

    Where a stack puts its children across its width.

    - `trx.ui.HAlign.LEFT`  
        Against the left edge.
    - `trx.ui.HAlign.CENTER`  
        In the middle.
    - `trx.ui.HAlign.RIGHT`  
        Against the right edge.
    - `trx.ui.HAlign.SPAN`  
        Stretched to the full width.
    - `trx.ui.HAlign.DISTRIBUTE`  
        Spread out, with the gaps taking the spare width.

- <a id="ui.VAlign" name="ui.VAlign"></a>[lua]`trx.ui.VAlign`

    Where a stack puts its children down its height.

    - `trx.ui.VAlign.TOP`  
        Against the top edge.
    - `trx.ui.VAlign.CENTER`  
        In the middle.
    - `trx.ui.VAlign.BOTTOM`  
        Against the bottom edge.
    - `trx.ui.VAlign.SPAN`  
        Stretched to the full height.
    - `trx.ui.VAlign.DISTRIBUTE`  
        Spread out, with the gaps taking the spare height.

- <a id="ui.Region" name="ui.Region"></a>[lua]`trx.ui.Region`

    One of the nine places the interface is built in. A handler is told which
    one is being built and adds to it, and everything asking for a place is laid
    out together there rather than over what else asked for it.

    The eight around the edge stack what they hold away from the edge they sit
    at. The middle is what the others leave, and is where a dialog goes.

    - `trx.ui.Region.TOP_LEFT`  
        The top left corner.
    - `trx.ui.Region.TOP_CENTER`  
        The top edge, in the middle.
    - `trx.ui.Region.TOP_RIGHT`  
        The top right corner.
    - `trx.ui.Region.LEFT`  
        The left edge, halfway down.
    - `trx.ui.Region.CENTER`  
        The middle of the screen, inside what the others leave.
    - `trx.ui.Region.RIGHT`  
        The right edge, halfway down.
    - `trx.ui.Region.BOTTOM_LEFT`  
        The bottom left corner.
    - `trx.ui.Region.BOTTOM_CENTER`  
        The bottom edge, in the middle.
    - `trx.ui.Region.BOTTOM_RIGHT`  
        The bottom right corner.

- <a id="ui.Layer" name="ui.Layer"></a>[lua]`trx.ui.Layer`

    Whether a widget is drawn below or above the engine interface.

    Widgets use the lower layer by default. Use the upper layer for a console or
    a text field. Each region keeps space for both layers.

    - `trx.ui.Layer.UNDER`  
        Below the engine interface.
    - `trx.ui.Layer.OVER`  
        Above the engine interface.

- <a id="ui.FrameStyle" name="ui.FrameStyle"></a>[lua]`trx.ui.FrameStyle`

    Which of the game's frames to draw. The look of each follows the menu style
    the player chose.

    - `trx.ui.FrameStyle.DIALOG`  
        The box a dialog sits in.
    - `trx.ui.FrameStyle.DIALOG_HEAVY`  
        The box a dialog sits in, drawn solid.
    - `trx.ui.FrameStyle.HEADING`  
        The strip a dialog puts its title in.
    - `trx.ui.FrameStyle.SELECTED`  
        The box around the option the player is on.
    - `trx.ui.FrameStyle.OUTLINE`  
        An outline with nothing behind it.

- <a id="ui.BarType" name="ui.BarType"></a>[lua]`trx.ui.BarType`

    Which of the game's bars to draw, which decides its colors.

    - `trx.ui.BarType.LARA_HP`  
        Lara's health.
    - `trx.ui.BarType.LARA_HP_POISON`  
        Lara's health while she is poisoned.
    - `trx.ui.BarType.LARA_AIR`  
        Lara's air.
    - `trx.ui.BarType.LARA_STAMINA`  
        Lara's stamina.
    - `trx.ui.BarType.LARA_EXPOSURE`  
        Lara's exposure to the cold.
    - `trx.ui.BarType.ENEMY_HP`  
        An enemy's health.
    - `trx.ui.BarType.ALLY_HP`  
        An ally's health.
    - `trx.ui.BarType.PROGRESS`  
        A general progress bar.

- <a id="ui.Screen" name="ui.Screen"></a>[lua]`trx.ui.Screen`

    An engine screen that a script can draw.

    - `trx.ui.Screen.RING_ENTRY`  
        An entry that the player uses in the inventory ring. The context reports the entry as [`trx.ui.ScreenContext.object`](#ui.ScreenContext.object). A definition can name the entry it draws. A ring opened to save or load leaves when the screen ends, and any ring leaves when the screen ends with [`trx.ui.ScreenContext:confirm`](#ui.ScreenContext.confirm).
    - `trx.ui.Screen.PAUSE`  
        The question that the pause screen asks when the player presses the inventory key: whether to leave for the title screen.
    - `trx.ui.Screen.SAVE_LOAD`  
        The quick save or load screen. The save and load keys open it when the instant screen setting is on. The context reports whether it opened for saving or loading as [`trx.ui.ScreenContext.mode`](#ui.ScreenContext.mode).

### Structures

- <a id="ui.Area" name="ui.Area"></a>[lua]`trx.ui.Area`

    A rectangle on the canvas, in canvas units, counted from the top left.

    Properties:
    - <a id="ui.Area.height" name="ui.Area.height"></a>**`height`**: number. How tall it is.
    - <a id="ui.Area.width" name="ui.Area.width"></a>**`width`**: number. How wide it is.
    - <a id="ui.Area.x" name="ui.Area.x"></a>**`x`**: number. The left edge.
    - <a id="ui.Area.y" name="ui.Area.y"></a>**`y`**: number. The top edge.

- <a id="ui.MeshSlot" name="ui.MeshSlot"></a>[lua]`trx.ui.MeshSlot`

    A model the interface keeps on screen across ticks. Move it once per tick;
    the engine blends between its current and previous poses when it draws each
    frame. The fields report the current tick's pose.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="ui.MeshSlot.h" name="ui.MeshSlot.h"></a>**`h`**: number. How tall the box is, in canvas units. *(read-only)*
    - <a id="ui.MeshSlot.object" name="ui.MeshSlot.object"></a>**`object`**: [trx.catalog.objects](CATALOG.md#catalog.objects). The object drawn in the slot. *(read-only)*
    - <a id="ui.MeshSlot.rot_x" name="ui.MeshSlot.rot_x"></a>**`rot_x`**: [trx.math.Angle](MATH.md#math.Angle). How far the model is tilted. *(read-only)*
    - <a id="ui.MeshSlot.rot_y" name="ui.MeshSlot.rot_y"></a>**`rot_y`**: [trx.math.Angle](MATH.md#math.Angle). How far the model is turned. *(read-only)*
    - <a id="ui.MeshSlot.rot_z" name="ui.MeshSlot.rot_z"></a>**`rot_z`**: [trx.math.Angle](MATH.md#math.Angle). How far the model is rolled. *(read-only)*
    - <a id="ui.MeshSlot.visible" name="ui.MeshSlot.visible"></a>**`visible`**: boolean. Whether the model is drawn. *(read-only)*
    - <a id="ui.MeshSlot.w" name="ui.MeshSlot.w"></a>**`w`**: number. How wide the box is, in canvas units. *(read-only)*
    - <a id="ui.MeshSlot.x" name="ui.MeshSlot.x"></a>**`x`**: number. The left edge of the box, in canvas units. *(read-only)*
    - <a id="ui.MeshSlot.y" name="ui.MeshSlot.y"></a>**`y`**: number. The top edge of the box, in canvas units. *(read-only)*

    Methods:

    - <a id="ui.MeshSlot.hide" name="ui.MeshSlot.hide"></a>[lua]`meshslot:hide()`  
      Stops drawing the model. Moving the slot again shows it.

    - <a id="ui.MeshSlot.move" name="ui.MeshSlot.move"></a>[lua]`meshslot:move()`  
      Puts the model where it should be at the end of this tick, and shows it.

      Takes a table of `object` , `x` ,
      `y` , `w` , `h` ,
      `rot_x` , `rot_y` and
      `rot_z` . The box uses canvas units. An omitted value is
      zero. Each turn takes the short way around the angle wrap.

      Call this once per tick. Calling it twice in one tick replaces the pose used
      for interpolation. A hidden slot, or one given a new object, starts at the
      new pose.

    - <a id="ui.MeshSlot.release" name="ui.MeshSlot.release"></a>[lua]`meshslot:release()`  
      Gives the slot back. The handle is spent afterwards, and moving or hiding a
      spent handle raises rather than reaching whichever slot came next. Releasing
      one again does nothing.

- <a id="ui.LayerKeys" name="ui.LayerKeys"></a>[lua]`trx.ui.LayerKeys`

    The player's input, as the top layer reads it.

    A layer reads each press as pressed once per tick. A menu key that the
    player holds keeps reading as pressed at the rate that the game's own menus
    repeat it. The presses that a layer read on the tick that it closes do not
    reach the layer below, because they stay inactive until the player releases
    them.

    Methods:

    - <a id="ui.LayerKeys.held" name="ui.LayerKeys.held"></a>[lua]`layerkeys:held(role)`  
      Returns whether a role is active. Does not use it up.

      Parameters:
      - <a id="ui.LayerKeys.held.role" name="ui.LayerKeys.held.role"></a>**`role`** ([trx.input.Role](INPUT.md#input.Role)). The role.

      Returns: boolean. Whether it is held.

    - <a id="ui.LayerKeys.held_for" name="ui.LayerKeys.held_for"></a>[lua]`layerkeys:held_for(role)`  
      Returns for how many ticks a role has been held. A tick in which the layer
      did not ask counts as a release.

      Parameters:
      - <a id="ui.LayerKeys.held_for.role" name="ui.LayerKeys.held_for.role"></a>**`role`** ([trx.input.Role](INPUT.md#input.Role)). The role.

      Returns: integer. The number of ticks.

    - <a id="ui.LayerKeys.pressed" name="ui.LayerKeys.pressed"></a>[lua]`layerkeys:pressed(role)`  
      Returns whether a role became active this tick, and uses the press up.

      Parameters:
      - <a id="ui.LayerKeys.pressed.role" name="ui.LayerKeys.pressed.role"></a>**`role`** ([trx.input.Role](INPUT.md#input.Role)). The role.

      Returns: boolean. Whether it was pressed.

- <a id="ui.StackLayer" name="ui.StackLayer"></a>[lua]`trx.ui.StackLayer`

    One screen of widgets on the stack.

    Properties:
    - <a id="ui.StackLayer.is_open" name="ui.StackLayer.is_open"></a>**`is_open`**: boolean. Whether the layer is still on the stack. *(read-only)*

    Methods:

    - <a id="ui.StackLayer.close" name="ui.StackLayer.close"></a>[lua]`stacklayer:close()`  
      Removes the layer from the stack, and releases its widgets. Does nothing if
      the layer is already closed.

      Returns: boolean. Whether the layer was open.

    - <a id="ui.StackLayer.is_top" name="ui.StackLayer.is_top"></a>[lua]`stacklayer:is_top()`  
      Returns whether the layer is the one that reads input.

      Returns: boolean. Whether it is on top.

    - <a id="ui.StackLayer.set_root" name="ui.StackLayer.set_root"></a>[lua]`stacklayer:set_root(root)`  
      Replaces the widget tree that the layer draws, and releases the old one.

      Parameters:
      - <a id="ui.StackLayer.set_root.root" name="ui.StackLayer.set_root.root"></a>**`root`** ([trx.ui.Widget](#ui.Widget)). The new tree.

- <a id="ui.ScreenContext" name="ui.ScreenContext"></a>[lua]`trx.ui.ScreenContext`

    A screen that a script holds, which the definition receives.

    Properties:
    - <a id="ui.ScreenContext.is_held" name="ui.ScreenContext.is_held"></a>**`is_held`**: boolean. Whether the script still holds the screen. *(read-only)*
    - <a id="ui.ScreenContext.mode" name="ui.ScreenContext.mode"></a>**`mode`**: [trx.inventory_ring.Mode](INVENTORY_RING.md#inventory_ring.Mode). What the quick save or load screen opened for, for [`trx.ui.Screen.SAVE_LOAD`](#ui.Screen). *(read-only)*
    - <a id="ui.ScreenContext.object" name="ui.ScreenContext.object"></a>**`object`**: [trx.catalog.objects](CATALOG.md#catalog.objects). The ring entry that the player uses, for [`trx.ui.Screen.RING_ENTRY`](#ui.Screen). *(read-only)*
    - <a id="ui.ScreenContext.screen" name="ui.ScreenContext.screen"></a>**`screen`**: [trx.ui.Screen](#ui.Screen). The screen. *(read-only)*

    Methods:

    - <a id="ui.ScreenContext.cancel" name="ui.ScreenContext.cancel"></a>[lua]`screencontext:cancel()`  
      Ends the screen, and closes its layers. A ring entry is put away, the pause
      screen stays paused and drops its question, and the quick save or load
      screen closes. Does nothing if the screen has already ended.

      Returns: boolean. Whether the screen was still held.

    - <a id="ui.ScreenContext.confirm" name="ui.ScreenContext.confirm"></a>[lua]`screencontext:confirm()`  
      Ends the screen as a choice that the player made, and closes its layers. A
      ring entry leaves the ring, as an entry that the player uses does. Does
      nothing if the screen has already ended.

      Returns: boolean. Whether the screen was still held.

    - <a id="ui.ScreenContext.exit_to_title" name="ui.ScreenContext.exit_to_title"></a>[lua]`screencontext:exit_to_title()`  
      Ends the pause screen, and leaves for the title screen with the pause
      screen's fade. Only [`trx.ui.Screen.PAUSE`](#ui.Screen) takes this.

      Returns: boolean. Whether the screen was still held.

    - <a id="ui.ScreenContext.push" name="ui.ScreenContext.push"></a>[lua]`screencontext:push(settings)`  
      Pushes a layer that belongs to the screen, with the settings that
      [`trx.ui.layers.push`](#ui.layers.push) takes. The layer closes when the screen ends.

      Parameters:
      - <a id="ui.ScreenContext.push.settings" name="ui.ScreenContext.push.settings"></a>**`settings`** (table). The layer settings.

      Returns: [trx.ui.StackLayer](#ui.StackLayer). The pushed layer.

    - <a id="ui.ScreenContext.resume" name="ui.ScreenContext.resume"></a>[lua]`screencontext:resume()`  
      Ends the pause screen, and returns to the game. Only [`trx.ui.Screen.PAUSE`](#ui.Screen)
      takes this.

      Returns: boolean. Whether the screen was still held.

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

      [`trx.ui.regions.place`](#ui.regions.place) calls this automatically. Custom layout code can call
      it during [`trx.events.on_ui_paint`](EVENTS.md#events.on_ui_paint).

      Parameters:
      - <a id="ui.Widget.paint.x" name="ui.Widget.paint.x"></a>**`x`** (number). The left edge.
      - <a id="ui.Widget.paint.y" name="ui.Widget.paint.y"></a>**`y`** (number). The top edge.
      - <a id="ui.Widget.paint.w" name="ui.Widget.paint.w"></a>**`w`** (number). The width it was given.
      - <a id="ui.Widget.paint.h" name="ui.Widget.paint.h"></a>**`h`** (number). The height it was given.

    - <a id="ui.Widget.release" name="ui.Widget.release"></a>[lua]`widget:release()`  
      Detaches the widget and its children from registered signals.

      Signals keep references to their listeners. Release temporary widgets when
      they are no longer needed. Remove a placed widget from its region before
      releasing it.

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

- <a id="ui.ListRow" name="ui.ListRow"></a>[lua]`trx.ui.ListRow`

    One entry of a [`trx.ui.widgets.List`](#ui.widgets.List).

    Properties:
    - <a id="ui.ListRow.right" name="ui.ListRow.right"></a>**`right`**: string, optional. Text drawn against the right edge. The main text is then drawn against the left edge.
    - <a id="ui.ListRow.rule" name="ui.ListRow.rule"></a>**`rule`**: boolean, optional. Whether a line separates the row from the row above it.
    - <a id="ui.ListRow.text" name="ui.ListRow.text"></a>**`text`**: string. The text. It is centered unless the row has a right part.

- <a id="ui.List" name="ui.List"></a>[lua]`trx.ui.List`

    A column of rows that the player picks one entry from. The row under the
    cursor is drawn in a frame. Arrows show where the list runs past the rows it
    shows.

    Methods:

    - <a id="ui.List.control" name="ui.List.control"></a>[lua]`list:control(keys)`  
      Reads the menu keys for one tick. Up and down move the cursor, and confirm
      picks the row under it. Uses up only the presses that it reads.

      Parameters:
      - <a id="ui.List.control.keys" name="ui.List.control.keys"></a>**`keys`** ([trx.ui.LayerKeys](#ui.LayerKeys)). The input of the layer the list is on.

      Returns: integer or `nil`. The picked row, or `nil` where none was picked.

    - <a id="ui.List.move" name="ui.List.move"></a>[lua]`list:move(step)`  
      Moves the cursor by a number of rows. Past either end, the cursor goes to
      the other end where the `ui.enable_wraparound` setting is on, and stays
      otherwise.

      Parameters:
      - <a id="ui.List.move.step" name="ui.List.move.step"></a>**`step`** (integer). How many rows to move. Negative moves up.

      Returns: boolean. Whether the cursor moved.

    - <a id="ui.List.select" name="ui.List.select"></a>[lua]`list:select(index)`  
      Moves the cursor to a row. Does nothing for an index out of range.

      Parameters:
      - <a id="ui.List.select.index" name="ui.List.select.index"></a>**`index`** (integer). The row.

    - <a id="ui.List.selection" name="ui.List.selection"></a>[lua]`list:selection()`  
      Returns the index of the row under the cursor.

      Returns: integer or `nil`. The index, or `nil` for an empty list.

    - <a id="ui.List.set_rows" name="ui.List.set_rows"></a>[lua]`list:set_rows(rows)`  
      Replaces the rows. The cursor stays on the same index where it can.

      Parameters:
      - <a id="ui.List.set_rows.rows" name="ui.List.set_rows.rows"></a>**`rows`** (a list of [trx.ui.ListRow](#ui.ListRow)). The new rows.

### Functions

- <a id="ui.layers" name="ui.layers"></a>[lua]`trx.ui.layers`  
  Draws screens of widgets over the rest of the interface.

  A layer holds one widget tree, such as a menu or a question. Layers are kept
  in a stack. Each layer draws over the layers below it and over the widgets
  placed with [`trx.ui.regions.place`](#ui.regions.place). The engine interface still draws over
  all of them.

  Only the top layer that takes input reads the player's input. The layers
  below it read nothing until it closes.

  A layer that a level script pushes closes when the level ends.

- <a id="ui.primitive" name="ui.primitive"></a>[lua]`trx.ui.primitive`  
  Low-level drawing calls and layout reservations.

  Use [`trx.ui.widgets`](#ui.widgets) for normal UI. Use these primitives only when building
  a custom widget. Primitive drawing does not affect region layout unless code
  reserves space first.

  Drawing calls are available only during [`trx.events.on_ui_paint`](EVENTS.md#events.on_ui_paint). They
  report an error at any other time.

- <a id="ui.regions" name="ui.regions"></a>[lua]`trx.ui.regions`  
  Places script widgets on the screen.

  The screen has nine regions. Engine UI uses those regions for bars, overlay
  text, inventory-ring hints, and dialogs. A widget placed in a region stacks
  after the engine UI in that region.

  Place a widget once when the script loads. Use signals when the widget must
  change later.

- <a id="ui.screens" name="ui.screens"></a>[lua]`trx.ui.screens`  
  Lets a script draw an engine screen in place of the engine.

  Define a screen with [`define`](#ui.screens.define). When the engine opens the
  screen, it calls the function that the definition gives. The function pushes
  layers through the context it receives and returns the first one. The engine
  then draws nothing for the screen and reads no input for it, until the
  script ends the screen through the context.

  The screen also ends when the layer that the definition returned closes, for
  any reason, and the screen's other layers close with it. A layer that raises
  an error therefore gives the screen back to the engine.

  While a script holds a screen, a game-flow command such as
  [`trx.savegame.load`](SAVEGAME.md#savegame.load) waits for the screen to end. In the inventory ring, the
  ring spins out before the command runs.

- <a id="ui.widgets" name="ui.widgets"></a>[lua]`trx.ui.widgets`  
  The widgets a script builds its screen from.

  A widget is created once and kept. Give it signals instead of fixed values,
  then register those signals with [`trx.ui.Widget:wakes_on`](#ui.Widget.wakes_on).

  Put a widget on screen with [`trx.ui.regions.place`](#ui.regions.place).

- <a id="ui.mesh_slot" name="ui.mesh_slot"></a>[lua]`trx.ui.mesh_slot()`  
  Takes a slot for a model the interface keeps on screen across ticks.

  Take a slot once, when a script loads, and give it back with
  [`trx.ui.MeshSlot:release`](#ui.MeshSlot.release) when nothing needs it. Returns nothing where every
  slot is taken.

  Returns: [trx.ui.MeshSlot](#ui.MeshSlot). The slot, or `nil` where none is free.

- <a id="ui.layers.push" name="ui.layers.push"></a>[lua]`trx.ui.layers.push(settings)`  
  Puts a layer on top of the stack.

  The layer reads no input on the tick it is pushed, because the press that
  opened it is often still active.

  Parameters:
  - <a id="ui.layers.push.settings" name="ui.layers.push.settings"></a>**`settings`** (table). The layer settings.

    Keys:
    - <a id="ui.layers.push.settings.root" name="ui.layers.push.settings.root"></a>**`root`** ([trx.ui.Widget](#ui.Widget)). The widget tree to draw.
    - <a id="ui.layers.push.settings.region" name="ui.layers.push.settings.region"></a>**`region`** ([trx.ui.Region](#ui.Region), optional). The region that the tree takes room in. The tree then stacks with the other widgets in that region. Without a region or a place, the tree is centered in [`trx.ui.safe_area`](#ui.safe_area).
    - <a id="ui.layers.push.settings.place" name="ui.layers.push.settings.place"></a>**`place`** (function, optional). Returns the top left corner of the tree, in canvas units. It receives the width and the height that the tree measures.
    - <a id="ui.layers.push.settings.modal" name="ui.layers.push.settings.modal"></a>**`modal`** (boolean, optional). Whether the layer reads input. `true` by default.
    - <a id="ui.layers.push.settings.on_input" name="ui.layers.push.settings.on_input"></a>**`on_input`** (function, optional). Runs once a tick while the layer is the top layer that reads input. It receives the layer and a [`trx.ui.LayerKeys`](#ui.LayerKeys). An error closes the layer.
    - <a id="ui.layers.push.settings.on_close" name="ui.layers.push.settings.on_close"></a>**`on_close`** (function, optional). Runs once when the layer closes, for any reason. It receives the layer.

  Returns: [trx.ui.StackLayer](#ui.StackLayer). The pushed layer.

  Example:
  ```lua
  local layer = trx.ui.layers.push({
    root = trx.ui.widgets.Label({ text = "Paused" }),
    on_input = function(layer, keys)
      if keys:pressed(trx.input.Role.MENU_BACK) then
        layer:close()
      end
    end,
  })
  ```

- <a id="ui.layers.top" name="ui.layers.top"></a>[lua]`trx.ui.layers.top()`  
  Returns the top layer that reads input.

  Returns: [trx.ui.StackLayer](#ui.StackLayer) or `nil`. The layer, or `nil` if no layer reads input.

- <a id="ui.layers.count" name="ui.layers.count"></a>[lua]`trx.ui.layers.count()`  
  Returns how many layers are on the stack.

  Returns: integer. The number of layers.

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

  The canvas is a fixed 640x480 grid, and the screen size depends on the
  player settings and window. Use this with [`to_canvas`](#ui.primitive.to_canvas) when
  geometry must align to whole screen pixels, such as an even border.

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

- <a id="ui.primitive.horizontal_line" name="ui.primitive.horizontal_line"></a>[lua]`trx.ui.primitive.horizontal_line(x0, x1, y, [z])`  
  Draws a horizontal rule in the selected menu style.

  Parameters:
  - <a id="ui.primitive.horizontal_line.x0" name="ui.primitive.horizontal_line.x0"></a>**`x0`** (number). The left end.
  - <a id="ui.primitive.horizontal_line.x1" name="ui.primitive.horizontal_line.x1"></a>**`x1`** (number). The right end.
  - <a id="ui.primitive.horizontal_line.y" name="ui.primitive.horizontal_line.y"></a>**`y`** (number). The vertical position.
  - <a id="ui.primitive.horizontal_line.z" name="ui.primitive.horizontal_line.z"></a>**`z`** (integer, optional). The draw order.

- <a id="ui.primitive.panel" name="ui.primitive.panel"></a>[lua]`trx.ui.primitive.panel(x, y, z, w, h, style)`  
  Draws the box the game draws behind a dialog, in the style the player chose.

  The look follows the menu style setting, so a panel drawn this way matches
  the game's own dialogs rather than standing apart from them.

  Parameters:
  - <a id="ui.primitive.panel.x" name="ui.primitive.panel.x"></a>**`x`** (number). The left edge.
  - <a id="ui.primitive.panel.y" name="ui.primitive.panel.y"></a>**`y`** (number). The top edge.
  - <a id="ui.primitive.panel.z" name="ui.primitive.panel.z"></a>**`z`** (integer). The draw order.
  - <a id="ui.primitive.panel.w" name="ui.primitive.panel.w"></a>**`w`** (number). The width.
  - <a id="ui.primitive.panel.h" name="ui.primitive.panel.h"></a>**`h`** (number). The height.
  - <a id="ui.primitive.panel.style" name="ui.primitive.panel.style"></a>**`style`** ([trx.ui.FrameStyle](#ui.FrameStyle)). Which of the game's frames to draw.

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

- <a id="ui.primitive.image" name="ui.primitive.image"></a>[lua]`trx.ui.primitive.image(path, x, y, w, h, [opacity])`  
  Draws an image file in a box on the canvas.

  The image is looked for where the game keeps its images, and stretches to
  fill the box, so a box of the image's own shape keeps that shape. The image
  draws under everything else the canvas holds, whatever order the calls come
  in.

  Returns whether the game has such an image, so a script can leave the space
  alone where it does not.

  Parameters:
  - <a id="ui.primitive.image.path" name="ui.primitive.image.path"></a>**`path`** (string). The image file, named from the images directory.
  - <a id="ui.primitive.image.x" name="ui.primitive.image.x"></a>**`x`** (number). The left edge.
  - <a id="ui.primitive.image.y" name="ui.primitive.image.y"></a>**`y`** (number). The top edge.
  - <a id="ui.primitive.image.w" name="ui.primitive.image.w"></a>**`w`** (number). The width.
  - <a id="ui.primitive.image.h" name="ui.primitive.image.h"></a>**`h`** (number). The height.
  - <a id="ui.primitive.image.opacity" name="ui.primitive.image.opacity"></a>**`opacity`** (number, optional). How solid the image is, from 0 to 1. `1` by default.

  Returns: boolean. Whether the image was there to draw.

  Example:
  ```lua
  trx.ui.primitive.image("uklogo.pak", 64, 0, 512, 256)
  ```

- <a id="ui.primitive.sprite_count" name="ui.primitive.sprite_count"></a>[lua]`trx.ui.primitive.sprite_count(object)`  
  Reports how many sprites an object has.

  An object the level did not load has none, and a model has none as well. Use
  this function to check whether [`sprite`](#ui.primitive.sprite) has anything to
  draw.

  Parameters:
  - <a id="ui.primitive.sprite_count.object" name="ui.primitive.sprite_count.object"></a>**`object`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The sprite object to count.

  Returns: integer. How many sprites it has.

- <a id="ui.primitive.sprite_bounds" name="ui.primitive.sprite_bounds"></a>[lua]`trx.ui.primitive.sprite_bounds(object, sprite_num)`  
  Reports the edges of one sprite of an object, in canvas units at a scale of
  one.

  The edges sit around the point the sprite is drawn at, so both left and top
  are usually negative. Multiply them by the scale the sprite is drawn at.

  Raises where the level did not load the object, so check
  `trx.objects.get(object).loaded` first.

  Parameters:
  - <a id="ui.primitive.sprite_bounds.object" name="ui.primitive.sprite_bounds.object"></a>**`object`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The sprite object to read from.
  - <a id="ui.primitive.sprite_bounds.sprite_num" name="ui.primitive.sprite_bounds.sprite_num"></a>**`sprite_num`** (integer). Which sprite of the object to read, counted from 0.

  Returns:
  - number. The left edge.
  - number. The top edge.
  - number. The right edge.
  - number. The bottom edge.

- <a id="ui.primitive.mesh_bounds" name="ui.primitive.mesh_bounds"></a>[lua]`trx.ui.primitive.mesh_bounds(object)`  
  Reports the box a model occupies, from the first frame of its first
  animation.

  The box sits around the point the model is drawn at, so the low edges are
  usually negative. A script fits a model into a box of its own by comparing
  the two.

  Returns nothing where the object carries no model, which is how a script
  tells whether it can draw one at all. Raises where the level did not load
  the object, so check `trx.objects.get(object).loaded` first.

  Parameters:
  - <a id="ui.primitive.mesh_bounds.object" name="ui.primitive.mesh_bounds.object"></a>**`object`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The model object to measure.

  Returns:
  - [trx.math.Distance](MATH.md#math.Distance). The low edge across, or `nil` where the object carries no model.
  - [trx.math.Distance](MATH.md#math.Distance). The low edge down.
  - [trx.math.Distance](MATH.md#math.Distance). The low edge into the screen.
  - [trx.math.Distance](MATH.md#math.Distance). The high edge across.
  - [trx.math.Distance](MATH.md#math.Distance). The high edge down.
  - [trx.math.Distance](MATH.md#math.Distance). The high edge into the screen.

- <a id="ui.primitive.sprite" name="ui.primitive.sprite"></a>[lua]`trx.ui.primitive.sprite(object, sprite_num, x, y, z, scale, color)`  
  Draws one sprite of an object on the canvas.

  Raises where the level did not load the object, so check
  `trx.objects.get(object).loaded` first.

  Parameters:
  - <a id="ui.primitive.sprite.object" name="ui.primitive.sprite.object"></a>**`object`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The sprite object to draw from.
  - <a id="ui.primitive.sprite.sprite_num" name="ui.primitive.sprite.sprite_num"></a>**`sprite_num`** (integer). Which sprite of the object to draw, counted from 0.
  - <a id="ui.primitive.sprite.x" name="ui.primitive.sprite.x"></a>**`x`** (number). The left edge.
  - <a id="ui.primitive.sprite.y" name="ui.primitive.sprite.y"></a>**`y`** (number). The top edge.
  - <a id="ui.primitive.sprite.z" name="ui.primitive.sprite.z"></a>**`z`** (integer). The draw order.
  - <a id="ui.primitive.sprite.scale" name="ui.primitive.sprite.scale"></a>**`scale`** (number). Multiplies the sprite size. At 1 the sprite draws at its own size on the canvas.
  - <a id="ui.primitive.sprite.color" name="ui.primitive.sprite.color"></a>**`color`** ([trx.math.Color](MATH.md#math.Color)). What color to tint it with.

  Example:
  ```lua
  trx.ui.primitive.sprite(
    trx.catalog.objects.assault_digits, 3, 100, 20, 0, 1,
    trx.math.color("ffffff"))
  ```

- <a id="ui.primitive.gradient_sprite" name="ui.primitive.gradient_sprite"></a>[lua]`trx.ui.primitive.gradient_sprite(object, sprite_num, x, y, z, scale, tl, tr, bl, br)`  
  Draws one sprite of an object, with a color at each corner.

  Raises where the level did not load the object, so check
  `trx.objects.get(object).loaded` first.

  Parameters:
  - <a id="ui.primitive.gradient_sprite.object" name="ui.primitive.gradient_sprite.object"></a>**`object`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The sprite object to draw from.
  - <a id="ui.primitive.gradient_sprite.sprite_num" name="ui.primitive.gradient_sprite.sprite_num"></a>**`sprite_num`** (integer). Which sprite of the object to draw, counted from 0.
  - <a id="ui.primitive.gradient_sprite.x" name="ui.primitive.gradient_sprite.x"></a>**`x`** (number). The left edge.
  - <a id="ui.primitive.gradient_sprite.y" name="ui.primitive.gradient_sprite.y"></a>**`y`** (number). The top edge.
  - <a id="ui.primitive.gradient_sprite.z" name="ui.primitive.gradient_sprite.z"></a>**`z`** (integer). The draw order.
  - <a id="ui.primitive.gradient_sprite.scale" name="ui.primitive.gradient_sprite.scale"></a>**`scale`** (number). Multiplies the sprite size. At 1 the sprite draws at its own size on the canvas.
  - <a id="ui.primitive.gradient_sprite.tl" name="ui.primitive.gradient_sprite.tl"></a>**`tl`** ([trx.math.Color](MATH.md#math.Color)). The top-left color.
  - <a id="ui.primitive.gradient_sprite.tr" name="ui.primitive.gradient_sprite.tr"></a>**`tr`** ([trx.math.Color](MATH.md#math.Color)). The top-right color.
  - <a id="ui.primitive.gradient_sprite.bl" name="ui.primitive.gradient_sprite.bl"></a>**`bl`** ([trx.math.Color](MATH.md#math.Color)). The bottom-left color.
  - <a id="ui.primitive.gradient_sprite.br" name="ui.primitive.gradient_sprite.br"></a>**`br`** ([trx.math.Color](MATH.md#math.Color)). The bottom-right color.

- <a id="ui.regions.place" name="ui.regions.place"></a>[lua]`trx.ui.regions.place(region, widget, [layer])`  
  Places a widget in a region.

  The layer decides whether the widget is covered by the engine interface or
  covers it. A widget is under it unless the call says otherwise. Each layer
  keeps room of its own in the region, so widgets on the two layers stack
  rather than sit on top of each other.

  If the region argument is a signal, the widget moves when the signal
  changes.

  Parameters:
  - <a id="ui.regions.place.region" name="ui.regions.place.region"></a>**`region`** (any). The target region, or a signal that holds one.
  - <a id="ui.regions.place.widget" name="ui.regions.place.widget"></a>**`widget`** ([trx.ui.Widget](#ui.Widget)). The widget to place.
  - <a id="ui.regions.place.layer" name="ui.regions.place.layer"></a>**`layer`** ([trx.ui.Layer](#ui.Layer), optional). Which layer to draw on. Defaults to [`trx.ui.Layer.UNDER`](#ui.Layer).

  Example:
  ```lua
  trx.ui.regions.place(trx.ui.Region.TOP_LEFT, health_bar)
  ```

  Example:
  ```lua
  trx.ui.regions.place(
    trx.ui.Region.BOTTOM_LEFT,
    console,
    trx.ui.Layer.OVER
  )
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

  A region with only non-shown widgets draws nothing. A fallback can reserve
  that empty place instead, for example the corner arrows shown when a bar is
  off screen. Each region has at most one fallback.

  Parameters:
  - <a id="ui.regions.fallback.region" name="ui.regions.fallback.region"></a>**`region`** ([trx.ui.Region](#ui.Region)). The target region.
  - <a id="ui.regions.fallback.widget" name="ui.regions.fallback.widget"></a>**`widget`** ([trx.ui.Widget](#ui.Widget)). The fallback widget.

- <a id="ui.screens.define" name="ui.screens.define"></a>[lua]`trx.ui.screens.define(screen, open, [options])`  
  Defines how a script draws a screen.

  The function receives a [`trx.ui.ScreenContext`](#ui.ScreenContext) when the engine opens the
  screen. It pushes the screen's layers through the context and returns the
  first one. Returning nothing leaves the screen to the engine.

  A screen has one definition. Defining it again is an error unless the
  options say `override = true`. The new definition then replaces the old one,
  which comes back when a level script's definition goes with its level.

  Parameters:
  - <a id="ui.screens.define.screen" name="ui.screens.define.screen"></a>**`screen`** ([trx.ui.Screen](#ui.Screen)). The screen.
  - <a id="ui.screens.define.open" name="ui.screens.define.open"></a>**`open`** (function). Runs when the engine opens the screen.
  - <a id="ui.screens.define.options" name="ui.screens.define.options"></a>**`options`** (table, optional). The definition options.

    Keys:
    - <a id="ui.screens.define.options.object" name="ui.screens.define.options.object"></a>**`object`** ([trx.catalog.objects](CATALOG.md#catalog.objects), optional). The ring entry that the definition draws, for [`trx.ui.Screen.RING_ENTRY`](#ui.Screen). Without it, the definition draws every entry that has no definition of its own.
    - <a id="ui.screens.define.options.override" name="ui.screens.define.options.override"></a>**`override`** (boolean, optional). Whether to replace a definition that exists. `false` by default.

  Example:
  ```lua
  trx.ui.screens.define(trx.ui.Screen.RING_ENTRY, function(ctx)
    return ctx:push({
      root = trx.ui.widgets.Label({ text = "North" }),
      on_input = function(_, keys)
        if keys:pressed(trx.input.Role.MENU_BACK) then
          ctx:cancel()
        end
      end,
    })
  end, { object = trx.catalog.objects.COMPASS_OPTION })
  ```

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

- <a id="ui.widgets.Custom" name="ui.widgets.Custom"></a>[lua]`trx.ui.widgets.Custom(settings)`  
  A widget that measures and draws itself through functions the script gives.

  Use it for drawing that the other widgets do not cover. Register the signals
  that the functions read with [`trx.ui.Widget:wakes_on`](#ui.Widget.wakes_on).

  Parameters:
  - <a id="ui.widgets.Custom.settings" name="ui.widgets.Custom.settings"></a>**`settings`** (table). The widget settings.

    Keys:
    - <a id="ui.widgets.Custom.settings.measure" name="ui.widgets.Custom.settings.measure"></a>**`measure`** (function). Returns the width and the height the widget wants, in canvas units.
    - <a id="ui.widgets.Custom.settings.paint" name="ui.widgets.Custom.settings.paint"></a>**`paint`** (function). Draws the widget with [`trx.ui.primitive`](#ui.primitive). It receives the left edge, the top edge, the width and the height of the box the widget was given.
    - <a id="ui.widgets.Custom.settings.shown" name="ui.widgets.Custom.settings.shown"></a>**`shown`** (any, optional). Whether the widget is shown, or a signal that holds that value.

  Returns: [trx.ui.Widget](#ui.Widget). The widget.

  Example:
  ```lua
  local mark = trx.ui.widgets.Custom({
    measure = function()
      return 8, 8
    end,
    paint = function(x, y, w, h)
      trx.ui.primitive.quad(x, y, 0, w, h, trx.math.color("#ffffff"))
    end,
  })
  ```

- <a id="ui.widgets.Digits" name="ui.widgets.Digits"></a>[lua]`trx.ui.widgets.Digits(settings)`  
  A line of text drawn from an object's sprites, one sprite per character.

  The object supplies the ten digits, then a colon, a full stop, a `T` and an
  `s`, in that order, which is how the assault course digits are laid out. A
  space and a dash move the pen without drawing.

  The widget measures nothing where the level did not load the object, so a
  script can keep it on screen for a level that has no digits.

  Parameters:
  - <a id="ui.widgets.Digits.settings" name="ui.widgets.Digits.settings"></a>**`settings`** (table). The digit settings.

    Keys:
    - <a id="ui.widgets.Digits.settings.object" name="ui.widgets.Digits.settings.object"></a>**`object`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The sprite object to draw the characters from.
    - <a id="ui.widgets.Digits.settings.text" name="ui.widgets.Digits.settings.text"></a>**`text`** (any). The text, or a signal carrying it.
    - <a id="ui.widgets.Digits.settings.color" name="ui.widgets.Digits.settings.color"></a>**`color`** (any). What color to draw the characters in, or a signal carrying one.
    - <a id="ui.widgets.Digits.settings.color_bottom" name="ui.widgets.Digits.settings.color_bottom"></a>**`color_bottom`** (any, optional). The color the characters fade to down their height. The main color by default, which draws them flat.
    - <a id="ui.widgets.Digits.settings.mark_color" name="ui.widgets.Digits.settings.mark_color"></a>**`mark_color`** (any, optional). What color to draw the `T` in. The main color by default.
    - <a id="ui.widgets.Digits.settings.mark_color_bottom" name="ui.widgets.Digits.settings.mark_color_bottom"></a>**`mark_color_bottom`** (any, optional). The color the `T` fades to. Its own color by default.
    - <a id="ui.widgets.Digits.settings.shown" name="ui.widgets.Digits.settings.shown"></a>**`shown`** (any, optional). Whether the digits are shown, or a signal that holds that value.

  Returns: [trx.ui.Widget](#ui.Widget). The digits.

  Example:
  ```lua
  trx.ui.widgets.Digits({
    object = trx.catalog.objects.assault_digits,
    text = timer:map(format_time),
    color = trx.math.color("ffffff"),
  })
  ```

- <a id="ui.widgets.Image" name="ui.widgets.Image"></a>[lua]`trx.ui.widgets.Image(settings)`  
  A picture from an image file, at a size the script gives.

  The widget keeps its room even where the game ships no such image, so a
  screen built around it does not move when the image is missing.

  Parameters:
  - <a id="ui.widgets.Image.settings" name="ui.widgets.Image.settings"></a>**`settings`** (table). The image settings.

    Keys:
    - <a id="ui.widgets.Image.settings.path" name="ui.widgets.Image.settings.path"></a>**`path`** (any). The image file, named from the images directory, or a signal carrying it.
    - <a id="ui.widgets.Image.settings.w" name="ui.widgets.Image.settings.w"></a>**`w`** (number). The width, in canvas units.
    - <a id="ui.widgets.Image.settings.h" name="ui.widgets.Image.settings.h"></a>**`h`** (number). The height, in canvas units.
    - <a id="ui.widgets.Image.settings.opacity" name="ui.widgets.Image.settings.opacity"></a>**`opacity`** (any, optional). How solid the image is, from 0 to 1, or a signal that holds that value. `1` by default.
    - <a id="ui.widgets.Image.settings.shown" name="ui.widgets.Image.settings.shown"></a>**`shown`** (any, optional). Whether the image is shown, or a signal that holds that value.

  Returns: [trx.ui.Widget](#ui.Widget). The image.

- <a id="ui.widgets.Label" name="ui.widgets.Label"></a>[lua]`trx.ui.widgets.Label(settings)`  
  A line of text. Use a signal for text that changes.

  Parameters:
  - <a id="ui.widgets.Label.settings" name="ui.widgets.Label.settings"></a>**`settings`** (table). The label settings.

    Keys:
    - <a id="ui.widgets.Label.settings.text" name="ui.widgets.Label.settings.text"></a>**`text`** (any). The text, or a signal carrying it.
    - <a id="ui.widgets.Label.settings.scale" name="ui.widgets.Label.settings.scale"></a>**`scale`** (number, optional). Multiplies the text size. `1.0` by default.
    - <a id="ui.widgets.Label.settings.shown" name="ui.widgets.Label.settings.shown"></a>**`shown`** (any, optional). Whether the label is shown, or a signal that holds that value.

  Returns: [trx.ui.Widget](#ui.Widget). The label.

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

- <a id="ui.widgets.Pad" name="ui.widgets.Pad"></a>[lua]`trx.ui.widgets.Pad(settings)`  
  Keeps a margin around a child widget.

  The margin is in canvas units at the default text size, and follows the text
  scale the same way the widgets inside it do.

  Parameters:
  - <a id="ui.widgets.Pad.settings" name="ui.widgets.Pad.settings"></a>**`settings`** (table). The padding settings.

    Keys:
    - <a id="ui.widgets.Pad.settings.child" name="ui.widgets.Pad.settings.child"></a>**`child`** ([trx.ui.Widget](#ui.Widget)). The child widget.
    - <a id="ui.widgets.Pad.settings.x" name="ui.widgets.Pad.settings.x"></a>**`x`** (number, optional). The margin at the left and the right. `0` by default.
    - <a id="ui.widgets.Pad.settings.y" name="ui.widgets.Pad.settings.y"></a>**`y`** (number, optional). The margin at the top and the bottom. `0` by default.
    - <a id="ui.widgets.Pad.settings.shown" name="ui.widgets.Pad.settings.shown"></a>**`shown`** (any, optional). Whether the padded widget is shown, or a signal that holds that value.

  Returns: [trx.ui.Widget](#ui.Widget). The padded widget.

- <a id="ui.widgets.Frame" name="ui.widgets.Frame"></a>[lua]`trx.ui.widgets.Frame(settings)`  
  Draws one of the game's frames behind a child widget.

  The frame takes the whole box the child asks for, so pad the child where the
  text would otherwise sit against the edge.

  Parameters:
  - <a id="ui.widgets.Frame.settings" name="ui.widgets.Frame.settings"></a>**`settings`** (table). The frame settings.

    Keys:
    - <a id="ui.widgets.Frame.settings.child" name="ui.widgets.Frame.settings.child"></a>**`child`** ([trx.ui.Widget](#ui.Widget)). The child widget.
    - <a id="ui.widgets.Frame.settings.style" name="ui.widgets.Frame.settings.style"></a>**`style`** ([trx.ui.FrameStyle](#ui.FrameStyle), optional). Which frame to draw. The dialog box by default.
    - <a id="ui.widgets.Frame.settings.z" name="ui.widgets.Frame.settings.z"></a>**`z`** (integer, optional). The draw order. `160` by default, which is behind text.
    - <a id="ui.widgets.Frame.settings.shown" name="ui.widgets.Frame.settings.shown"></a>**`shown`** (any, optional). Whether the framed widget is shown, or a signal that holds that value.

  Returns: [trx.ui.Widget](#ui.Widget). The framed widget.

- <a id="ui.widgets.Fit" name="ui.widgets.Fit"></a>[lua]`trx.ui.widgets.Fit(settings)`  
  Shrinks a child widget until it is within the screen.

  Text keeps the size the player chose while it fits, and everything below
  this widget is drawn smaller where it does not. A dialog that has to hold a
  fixed body on a small screen wants this; a line of text that can simply wrap
  does not.

  Parameters:
  - <a id="ui.widgets.Fit.settings" name="ui.widgets.Fit.settings"></a>**`settings`** (table). The fit settings.

    Keys:
    - <a id="ui.widgets.Fit.settings.child" name="ui.widgets.Fit.settings.child"></a>**`child`** ([trx.ui.Widget](#ui.Widget)). The child widget.
    - <a id="ui.widgets.Fit.settings.shown" name="ui.widgets.Fit.settings.shown"></a>**`shown`** (any, optional). Whether the fitted widget is shown, or a signal that holds that value.

  Returns: [trx.ui.Widget](#ui.Widget). The fitted widget.

- <a id="ui.widgets.Row" name="ui.widgets.Row"></a>[lua]`trx.ui.widgets.Row(settings)`  
  A widget with a left and right arrow beside a child widget.

  Unlit arrows stay hidden but keep their room, so the child widget does not
  move when arrows appear or disappear.

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

- <a id="ui.widgets.List" name="ui.widgets.List"></a>[lua]`trx.ui.widgets.List(settings)`  
  A column of rows that the player picks one entry from.

  The list keeps the cursor and the scroll position. Read the player's input
  with [`trx.ui.List:control`](#ui.List.control) from the input callback of the layer that holds
  the list.

  Parameters:
  - <a id="ui.widgets.List.settings" name="ui.widgets.List.settings"></a>**`settings`** (table). The list settings.

    Keys:
    - <a id="ui.widgets.List.settings.rows" name="ui.widgets.List.settings.rows"></a>**`rows`** (a list of [trx.ui.ListRow](#ui.ListRow), optional). The rows. None by default.
    - <a id="ui.widgets.List.settings.visible" name="ui.widgets.List.settings.visible"></a>**`visible`** (any, optional). How many rows to show at once, or a signal that holds that value. Every row by default.
    - <a id="ui.widgets.List.settings.reserve" name="ui.widgets.List.settings.reserve"></a>**`reserve`** (boolean, optional). Whether to keep room for the visible rows when the list holds fewer. `false` by default.
    - <a id="ui.widgets.List.settings.width" name="ui.widgets.List.settings.width"></a>**`width`** (number, optional). The least width, in canvas units at the default text size.
    - <a id="ui.widgets.List.settings.row_pad" name="ui.widgets.List.settings.row_pad"></a>**`row_pad`** (number, optional). The room on each side of a row's text. `4` by default.
    - <a id="ui.widgets.List.settings.row_spacing" name="ui.widgets.List.settings.row_spacing"></a>**`row_spacing`** (number, optional). The gap between two rows. `3` by default.
    - <a id="ui.widgets.List.settings.scroll_hints" name="ui.widgets.List.settings.scroll_hints"></a>**`scroll_hints`** (boolean, optional). Whether to show arrows where the list runs past its rows. `true` by default.
    - <a id="ui.widgets.List.settings.shown" name="ui.widgets.List.settings.shown"></a>**`shown`** (any, optional). Whether the list is shown, or a signal that holds that value.

  Returns: [trx.ui.List](#ui.List). The list.

  Example:
  ```lua
  local list = trx.ui.widgets.List({
    rows = { { text = "Yes" }, { text = "No" } },
  })
  ```

- <a id="ui.widgets.SleekBar" name="ui.widgets.SleekBar"></a>[lua]`trx.ui.widgets.SleekBar(settings)`  
  A thin bar that shows progress, as the game draws it under a button that the
  player holds.

  The bar is a dark frame with a fill in the game's own colour. It takes the
  width of the box that it is given, and its height follows the text size. Use
  a signal for progress that changes.

  Parameters:
  - <a id="ui.widgets.SleekBar.settings" name="ui.widgets.SleekBar.settings"></a>**`settings`** (table). The bar settings.

    Keys:
    - <a id="ui.widgets.SleekBar.settings.progress" name="ui.widgets.SleekBar.settings.progress"></a>**`progress`** (any). How full the bar is, from 0 to 1, or a signal that holds it.
    - <a id="ui.widgets.SleekBar.settings.shown" name="ui.widgets.SleekBar.settings.shown"></a>**`shown`** (any, optional). Whether the bar is shown, or a signal that holds that value.

  Returns: [trx.ui.Widget](#ui.Widget). The bar.

  Example:
  ```lua
  local held = trx.signal.new(0)
  local bar = trx.ui.widgets.SleekBar({ progress = held })
  ```
