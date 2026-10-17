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

A script builds its interface from widgets once, and keeps them.
[`trx.ui.regions.place`](#ui.regions.place) puts a widget in one of the regions of the screen,
where it stacks with the health bars and the item names, so the player's
choice of where each element sits still holds. [`trx.ui.layers.push`](#ui.layers.push) puts a
screen of widgets, such as a menu, over the rest. A widget takes signals in
place of fixed values, and follows them as they change.

Below the widgets, a script can reserve room in a region during
[`trx.ui.on_reserve`](#ui.on_reserve) and paint into it by hand with [`trx.ui.primitive`](#ui.primitive) during
[`trx.ui.on_paint`](#ui.on_paint). The primitive drawing calls work only while the
interface is painted.

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
- <a id="ui.screen" name="ui.screen"></a>**`trx.ui.screen`** ([trx.ui.Area](#ui.Area)). The whole screen, in canvas units. The canvas holds a whole number of units, so its right and bottom edges can stop short of the screen by a fraction of a unit; the screen reaches the edges. Something drawn from edge to edge, such as a frame around the screen, uses this area. *(read-only)*
- <a id="ui.safe_area" name="ui.safe_area"></a>**`trx.ui.safe_area`** ([trx.ui.Area](#ui.Area)). The part of the canvas that is free to draw in: the canvas, less the margin kept at the edges, less what the game reserves at the top and the bottom for the bars and the text it puts there. *(read-only)*
- <a id="ui.working_area" name="ui.working_area"></a>**`trx.ui.working_area`** ([trx.ui.Area](#ui.Area)). The part of the canvas between the cinematic bars. It is the whole canvas while there are none, and follows the bars as they move in and out. The interface regions lay out inside it, so [`trx.ui.safe_area`](#ui.safe_area) lies within it too. *(read-only)*
- <a id="ui.signals.layout" name="ui.signals.layout"></a>**`trx.ui.signals.layout`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when the room that the interface lays out in changes: the canvas, [`trx.ui.safe_area`](#ui.safe_area), [`trx.ui.working_area`](#ui.working_area) or [`trx.ui.text_scale`](#ui.text_scale). It holds a count that goes up with each change. A widget that lays itself out by that room listens to this rather than reading the room every frame. *(read-only)*

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

    One of the nine areas of the screen that the interface is laid out in.
    Everything that reserves room in a region is stacked together, so nothing in
    one region overlaps.

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
    - `trx.ui.FrameStyle.HEADING_OUTLINE`  
        The outline of the strip a dialog puts its title in, with nothing behind it, as the game draws around a color swatch.

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

    An engine screen that a script can draw. Each screen hands its definition
    a context of its own kind, which reports what the screen shows.

    - `trx.ui.Screen.RING_ENTRY`  
        An entry that the player uses in the inventory ring, with a [`trx.ui.EntryContext`](#ui.EntryContext). A definition can name the entry it draws. A ring opened to save or load leaves when the screen ends, and any ring leaves when the screen ends with [`trx.ui.ScreenContext:confirm`](#ui.ScreenContext.confirm).
    - `trx.ui.Screen.PAUSE`  
        The pause screen, from the moment the game pauses until it resumes or leaves for the title screen. It has a [`trx.ui.PauseContext`](#ui.PauseContext).
    - `trx.ui.Screen.SAVE_LOAD`  
        The quick save or load screen, with a [`trx.ui.SaveLoadContext`](#ui.SaveLoadContext). The save and load keys open it when the instant screen setting is on.
    - `trx.ui.Screen.STATS`  
        The statistics that the game shows when a level ends, and the totals it shows when the game ends, with a [`trx.ui.StatsContext`](#ui.StatsContext). The player skips the screen as they would the engine's, and the screen also ends with [`trx.ui.ScreenContext:cancel`](#ui.ScreenContext.cancel) or [`trx.ui.ScreenContext:confirm`](#ui.ScreenContext.confirm).
    - `trx.ui.Screen.EXAMINE`  
        An entry that the player examines in the inventory ring: its name and description, and turning it by hand. It has a [`trx.ui.EntryContext`](#ui.EntryContext). The screen ends with [`trx.ui.ScreenContext:cancel`](#ui.ScreenContext.cancel), which puts the entry away.
    - `trx.ui.Screen.COMBINE`  
        An entry that the player combines in the inventory ring: picking what to combine it with, out of [`trx.inventory.combine_partners`](INVENTORY.md#inventory.combine_partners), and combining the two with [`trx.inventory.combine`](INVENTORY.md#inventory.combine). It has a [`trx.ui.EntryContext`](#ui.EntryContext). The screen ends with [`trx.ui.ScreenContext:cancel`](#ui.ScreenContext.cancel) or [`trx.ui.ScreenContext:confirm`](#ui.ScreenContext.confirm), which both put the entry away, and the ring then rests on what a combination made.
    - `trx.ui.Screen.GLOBE_SELECT`  
        The globe that TR3 picks its next adventure on, between levels, with a [`trx.ui.GlobeContext`](#ui.GlobeContext). The script picks an adventure by starting its level with [`trx.game.play_level`](GAME.md#game.play_level), and [`trx.ui.ScreenContext:cancel`](#ui.ScreenContext.cancel) leaves for the title screen.
    - `trx.ui.Screen.LEVEL_VIEW`  
        A view of the level from one of its cameras, which the game shows while the level starts, as TR4 does. It has a [`trx.ui.TimedContext`](#ui.TimedContext). The view ends with [`trx.ui.ScreenContext:confirm`](#ui.ScreenContext.confirm) or [`trx.ui.ScreenContext:cancel`](#ui.ScreenContext.cancel).
    - `trx.ui.Screen.PICTURE`  
        A picture that the game shows between levels, while a level loads, or for the legal notice and the credits, with a [`trx.ui.PictureContext`](#ui.PictureContext). The picture ends with [`trx.ui.ScreenContext:confirm`](#ui.ScreenContext.confirm) or [`trx.ui.ScreenContext:cancel`](#ui.ScreenContext.cancel).

### Structures

- <a id="ui.Inputs" name="ui.Inputs"></a>[lua]`trx.ui.Inputs`

    One input role, or a list of roles shown side by side.

- <a id="ui.Area" name="ui.Area"></a>[lua]`trx.ui.Area`

    A rectangle on the canvas, in canvas units, counted from the top left.

    Properties:
    - <a id="ui.Area.height" name="ui.Area.height"></a>**`height`**: number. How tall it is.
    - <a id="ui.Area.width" name="ui.Area.width"></a>**`width`**: number. How wide it is.
    - <a id="ui.Area.x" name="ui.Area.x"></a>**`x`**: number. The left edge.
    - <a id="ui.Area.y" name="ui.Area.y"></a>**`y`**: number. The top edge.

- <a id="ui.LayerKeys" name="ui.LayerKeys"></a>[lua]`trx.ui.LayerKeys`

    The player's input, as the active layer reads it.

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

    - <a id="ui.StackLayer.set_root" name="ui.StackLayer.set_root"></a>[lua]`stacklayer:set_root(root, [place])`  
      Replaces the widget tree that the layer draws, and releases the old one.

      Parameters:
      - <a id="ui.StackLayer.set_root.root" name="ui.StackLayer.set_root.root"></a>**`root`** ([trx.ui.Widget](#ui.Widget)). The new tree.
      - <a id="ui.StackLayer.set_root.place" name="ui.StackLayer.set_root.place"></a>**`place`** ([trx.ui.Placement](#ui.Placement) or function, optional). Where the new tree goes, as [`trx.ui.layers.push`](#ui.layers.push) takes it. The tree keeps the old placement where this is `nil`.

- <a id="ui.Placement" name="ui.Placement"></a>[lua]`trx.ui.Placement`

    Where a layer puts its tree within an area of the canvas. The tree takes a
    share of the room that it leaves free on each axis, and starts from the top
    left of the area where it is too large for it.

    Properties:
    - <a id="ui.Placement.canvas" name="ui.Placement.canvas"></a>**`canvas`**: any, optional. Whether the area is the whole canvas, or a signal that holds that value. By default, the area is [`trx.ui.safe_area_for`](#ui.safe_area_for) the width of the tree, as the engine centers its own boxes.
    - <a id="ui.Placement.margin" name="ui.Placement.margin"></a>**`margin`**: any, optional. The room kept clear inside each edge of the area, in canvas units, or a signal that holds it. `0` by default.
    - <a id="ui.Placement.x" name="ui.Placement.x"></a>**`x`**: any, optional. How far across the free width the tree stands, from `0` against the left edge to `1` against the right edge, or a signal that holds it. `0.5` by default.
    - <a id="ui.Placement.y" name="ui.Placement.y"></a>**`y`**: any, optional. How far down the free height the tree stands, from `0` against the top edge to `1` against the bottom edge, or a signal that holds it. `0.5` by default.

- <a id="ui.FadeLayer" name="ui.FadeLayer"></a>[lua]`trx.ui.FadeLayer`

    A layer drawn over the interface: one colour, or the frame as it was when
    the layer was made. Its opacity moves on a curve, and the layer paints
    itself, so the script that made it paints nothing. Fade layers stack in the
    order they were made, above every screen and below the console.

    A backdrop layer is drawn under the interface instead, as the backdrop that
    the game draws behind its screens. Its opacity says how far the backdrop has
    come in: a picture comes in from black, and any other style from the frame
    behind it.

    Properties:
    - <a id="ui.FadeLayer.holds_screen" name="ui.FadeLayer.holds_screen"></a>**`holds_screen`**: boolean. Whether the engine waits to start its next screen, or to exit, while the layer is open. Setting it to `false` lets the engine go on while the layer stays drawn.
    - <a id="ui.FadeLayer.is_open" name="ui.FadeLayer.is_open"></a>**`is_open`**: boolean. Whether the layer is still drawn. *(read-only)*
    - <a id="ui.FadeLayer.opacity" name="ui.FadeLayer.opacity"></a>**`opacity`**: number. How opaque the layer is now, from `0` to `1`. *(read-only)*

    Methods:

    - <a id="ui.FadeLayer.close" name="ui.FadeLayer.close"></a>[lua]`fadelayer:close()`  
      Stops drawing the layer. Does nothing if the layer is already closed.

      Returns: boolean. Whether the layer was open.

    - <a id="ui.FadeLayer.to" name="ui.FadeLayer.to"></a>[lua]`fadelayer:to(target, ticks, [options])`  
      Moves the opacity to a target. The move takes the ticks it is given whatever
      the fade settings are.

      Parameters:
      - <a id="ui.FadeLayer.to.target" name="ui.FadeLayer.to.target"></a>**`target`** (number). The opacity to go to, from `0` to `1`.
      - <a id="ui.FadeLayer.to.ticks" name="ui.FadeLayer.to.ticks"></a>**`ticks`** (integer). How many ticks the move takes.
      - <a id="ui.FadeLayer.to.options" name="ui.FadeLayer.to.options"></a>**`options`** (table, optional). How the move goes, and what happens when it ends.

        Keys:
        - <a id="ui.FadeLayer.to.options.curve" name="ui.FadeLayer.to.options.curve"></a>**`curve`** (function, optional). The curve of the move, one of [`trx.ui.ease`](#ui.ease). [`trx.ui.ease.linear`](#ui.ease.linear) by default.
        - <a id="ui.FadeLayer.to.options.close" name="ui.FadeLayer.to.options.close"></a>**`close`** (boolean, optional). Whether the layer closes once the move ends. `false` by default.
        - <a id="ui.FadeLayer.to.options.on_done" name="ui.FadeLayer.to.options.on_done"></a>**`on_done`** (function, optional). Runs once when the move ends, and not if another move or a close cuts it short. A move of no ticks made in a tick handler ends within that tick. It receives the layer.

      Returns: [trx.ui.FadeLayer](#ui.FadeLayer). The same layer.

- <a id="ui.regions.place.options" name="ui.regions.place.options"></a>[lua]`trx.ui.regions.place.options`

    How a widget stands among the others in its region.

    Properties:
    - <a id="ui.regions.place.options.alone" name="ui.regions.place.options.alone"></a>**`alone`**: boolean, optional. Whether the widget shows only while nothing else in its region does, as an arrow in a corner that a bar can take. `false` by default.
    - <a id="ui.regions.place.options.order" name="ui.regions.place.options.order"></a>**`order`**: number, optional. Where the widget stacks: a lower order stacks first, and widgets of the same order stack in the order they were placed. `0` by default.

- <a id="ui.ScreenContext" name="ui.ScreenContext"></a>[lua]`trx.ui.ScreenContext`

    A screen that a script holds, which the definition receives. Each screen
    hands over a context of its own kind, which adds what that screen reports to
    what every screen has.

    Properties:
    - <a id="ui.ScreenContext.is_held" name="ui.ScreenContext.is_held"></a>**`is_held`**: boolean. Whether the script still holds the screen. *(read-only)*
    - <a id="ui.ScreenContext.screen" name="ui.ScreenContext.screen"></a>**`screen`**: [trx.ui.Screen](#ui.Screen). The screen. *(read-only)*

    Methods:

    - <a id="ui.ScreenContext.cancel" name="ui.ScreenContext.cancel"></a>[lua]`screencontext:cancel()`  
      Ends the screen, and closes its layers. A ring entry is put away, the pause
      screen returns to the game, the quick save or load screen
      closes, and the statistics screen ends as the player skipping it does. Does
      nothing if the screen has already ended.

      Returns: boolean. Whether the screen was still held.

    - <a id="ui.ScreenContext.confirm" name="ui.ScreenContext.confirm"></a>[lua]`screencontext:confirm()`  
      Ends the screen as a choice that the player made, and closes its layers. A
      ring entry leaves the ring, as an entry that the player uses does, a
      combined entry is put away, and the statistics screen ends as the player
      skipping it does. Does nothing if the screen has already ended.

      Returns: boolean. Whether the screen was still held.

    - <a id="ui.ScreenContext.on_end" name="ui.ScreenContext.on_end"></a>[lua]`screencontext:on_end(callback)`  
      Runs a function once the screen ends, for any reason, after its layers have
      closed.

      Parameters:
      - <a id="ui.ScreenContext.on_end.callback" name="ui.ScreenContext.on_end.callback"></a>**`callback`** (function). What to run.

      Example:
      ```lua
      local black = trx.ui.layers.fade({ fill = trx.math.color("000000") })
      ctx:on_end(function()
        black:close()
      end)
      ```

    - <a id="ui.ScreenContext.push" name="ui.ScreenContext.push"></a>[lua]`screencontext:push(settings)`  
      Pushes a layer that belongs to the screen, with the settings that
      [`trx.ui.layers.push`](#ui.layers.push) takes. The layer closes when the screen ends.

      Parameters:
      - <a id="ui.ScreenContext.push.settings" name="ui.ScreenContext.push.settings"></a>**`settings`** (table). The layer settings.

        Keys:
        - <a id="ui.ScreenContext.push.settings.root" name="ui.ScreenContext.push.settings.root"></a>**`root`** ([trx.ui.Widget](#ui.Widget)). The widget tree to draw.
        - <a id="ui.ScreenContext.push.settings.place" name="ui.ScreenContext.push.settings.place"></a>**`place`** ([trx.ui.Placement](#ui.Placement) or function, optional). Where the tree goes. A function receives the width and the height that the tree measures, and returns the top left corner of the tree, in canvas units. The layer works the corner out again only when the tree's size, the room the interface lays out in, or a signal that the placement reads changes, and on every frame while the placement reads a [`trx.ui.Tween`](#ui.Tween). The tree is centered in [`trx.ui.safe_area_for`](#ui.safe_area_for) its width by default.
        - <a id="ui.ScreenContext.push.settings.modal" name="ui.ScreenContext.push.settings.modal"></a>**`modal`** (boolean, optional). Whether the layer reads input. A layer that reads input takes it from the layers below, and during play from the game. By default, a layer reads input when it has `on_input`. Set it to `true` for a layer that holds the input back without reading any.
        - <a id="ui.ScreenContext.push.settings.on_input" name="ui.ScreenContext.push.settings.on_input"></a>**`on_input`** (function, optional). Runs once a tick while the layer is the active layer. It receives the layer and a [`trx.ui.LayerKeys`](#ui.LayerKeys). An error closes the layer.
        - <a id="ui.ScreenContext.push.settings.on_close" name="ui.ScreenContext.push.settings.on_close"></a>**`on_close`** (function, optional). Runs once when the layer closes, for any reason. It receives the layer.

      Returns: [trx.ui.StackLayer](#ui.StackLayer). The pushed layer.

- <a id="ui.EntryContext" name="ui.EntryContext"></a>[lua]`trx.ui.EntryContext`

    The context of an entry of the inventory ring, for
    [`trx.ui.Screen.RING_ENTRY`](#ui.Screen), [`trx.ui.Screen.EXAMINE`](#ui.Screen) and
    [`trx.ui.Screen.COMBINE`](#ui.Screen).

    Properties:
    - <a id="ui.EntryContext.object" name="ui.EntryContext.object"></a>**`object`**: [trx.catalog.objects](CATALOG.md#catalog.objects). The entry that the player uses, examines or combines. *(read-only)*

- <a id="ui.PauseContext" name="ui.PauseContext"></a>[lua]`trx.ui.PauseContext`

    The context of the pause screen, for [`trx.ui.Screen.PAUSE`](#ui.Screen).

    Methods:

    - <a id="ui.PauseContext.exit_to_title" name="ui.PauseContext.exit_to_title"></a>[lua]`pausecontext:exit_to_title()`  
      Ends the pause screen, and leaves for the title screen.

      Returns: boolean. Whether the screen was still held.

    - <a id="ui.PauseContext.resume" name="ui.PauseContext.resume"></a>[lua]`pausecontext:resume()`  
      Ends the pause screen, and returns to the game.

      Returns: boolean. Whether the screen was still held.

- <a id="ui.SaveLoadContext" name="ui.SaveLoadContext"></a>[lua]`trx.ui.SaveLoadContext`

    The context of the quick save or load screen, for
    [`trx.ui.Screen.SAVE_LOAD`](#ui.Screen).

    Properties:
    - <a id="ui.SaveLoadContext.mode" name="ui.SaveLoadContext.mode"></a>**`mode`**: [trx.inventory_ring.Mode](INVENTORY_RING.md#inventory_ring.Mode). Whether the screen opened for saving or for loading. *(read-only)*

- <a id="ui.StatsContext" name="ui.StatsContext"></a>[lua]`trx.ui.StatsContext`

    The context of the statistics screen, for [`trx.ui.Screen.STATS`](#ui.Screen).

    Properties:
    - <a id="ui.StatsContext.background" name="ui.StatsContext.background"></a>**`background`**: string. The picture the screen stands on, as [`trx.ui.primitive.background`](#ui.primitive.background) takes it, or `nil` for none. *(read-only)*
    - <a id="ui.StatsContext.is_bare" name="ui.StatsContext.is_bare"></a>**`is_bare`**: boolean. Whether the screen is drawn bare, as plain lines of text without a frame. *(read-only)*
    - <a id="ui.StatsContext.is_final" name="ui.StatsContext.is_final"></a>**`is_final`**: boolean. Whether the screen shows the totals for the whole game rather than one level. *(read-only)*
    - <a id="ui.StatsContext.level" name="ui.StatsContext.level"></a>**`level`**: [trx.game.Level](GAME.md#game.Level). The level whose statistics the screen shows. The totals name the level that ended the game. *(read-only)*

- <a id="ui.GlobeContext" name="ui.GlobeContext"></a>[lua]`trx.ui.GlobeContext`

    The context of the TR3 globe, for [`trx.ui.Screen.GLOBE_SELECT`](#ui.Screen).

    Properties:
    - <a id="ui.GlobeContext.background" name="ui.GlobeContext.background"></a>**`background`**: string. The picture the globe stands on, as [`trx.ui.primitive.background`](#ui.primitive.background) takes it, or `nil` for none. *(read-only)*

- <a id="ui.TimedContext" name="ui.TimedContext"></a>[lua]`trx.ui.TimedContext`

    The context of a screen that the game shows for a time, for
    [`trx.ui.Screen.LEVEL_VIEW`](#ui.Screen).

    Properties:
    - <a id="ui.TimedContext.display_ticks" name="ui.TimedContext.display_ticks"></a>**`display_ticks`**: integer. How many ticks the screen shows before it fades out. *(read-only)*
    - <a id="ui.TimedContext.fade_in_ticks" name="ui.TimedContext.fade_in_ticks"></a>**`fade_in_ticks`**: integer. How many ticks the screen takes to fade in. *(read-only)*
    - <a id="ui.TimedContext.fade_out_ticks" name="ui.TimedContext.fade_out_ticks"></a>**`fade_out_ticks`**: integer. How many ticks the screen takes to fade out. *(read-only)*

- <a id="ui.PictureContext" name="ui.PictureContext"></a>[lua]`trx.ui.PictureContext`

    The context of a picture, for [`trx.ui.Screen.PICTURE`](#ui.Screen).

    Properties:
    - <a id="ui.PictureContext.background" name="ui.PictureContext.background"></a>**`background`**: string. The picture, as [`trx.ui.primitive.background`](#ui.primitive.background) takes it, or `nil` for none. *(read-only)*
    - <a id="ui.PictureContext.can_cross_fade_in" name="ui.PictureContext.can_cross_fade_in"></a>**`can_cross_fade_in`**: boolean. Whether the picture may fade in from the frame shown before it, rather than from black. *(read-only)*
    - <a id="ui.PictureContext.is_loading" name="ui.PictureContext.is_loading"></a>**`is_loading`**: boolean. Whether the picture shows while a level loads. *(read-only)*

- <a id="ui.Tween" name="ui.Tween"></a>[lua]`trx.ui.Tween`

    A number that moves to a target over some ticks.

    Reading it while the interface is painted gives the value part way between
    two ticks, so that what it moves glides at any frame rate.

    Methods:

    - <a id="ui.Tween.get" name="ui.Tween.get"></a>[lua]`tween:get()`  
      The value now.

      Returns: number. The value.

    - <a id="ui.Tween.is_moving" name="ui.Tween.is_moving"></a>[lua]`tween:is_moving()`  
      Whether the value is still on its way to its target, counted in whole
      ticks, as the input a script reads each tick is.

      Returns: boolean. `true` until the tick the move ends on.

    - <a id="ui.Tween.set" name="ui.Tween.set"></a>[lua]`tween:set(value)`  
      Puts the value at once where it is told, and stops any move.

      Parameters:
      - <a id="ui.Tween.set.value" name="ui.Tween.set.value"></a>**`value`** (number). The value.

      Returns: [trx.ui.Tween](#ui.Tween). The same tween.

    - <a id="ui.Tween.target" name="ui.Tween.target"></a>[lua]`tween:target()`  
      Where the value is going, or where it rests.

      Returns: number. The target.

    - <a id="ui.Tween.to" name="ui.Tween.to"></a>[lua]`tween:to(target, ticks, [options])`  
      Starts moving to a target from wherever the value is now.

      Parameters:
      - <a id="ui.Tween.to.target" name="ui.Tween.to.target"></a>**`target`** (number). Where to go.
      - <a id="ui.Tween.to.ticks" name="ui.Tween.to.ticks"></a>**`ticks`** (integer). How many ticks the move takes.
      - <a id="ui.Tween.to.options" name="ui.Tween.to.options"></a>**`options`** (table, optional). How the move goes.

        Keys:
        - <a id="ui.Tween.to.options.curve" name="ui.Tween.to.options.curve"></a>**`curve`** (function, optional). The curve of the move, one of [`trx.ui.ease`](#ui.ease). [`trx.ui.ease.linear`](#ui.ease.linear) by default. Any other function raises an error.
        - <a id="ui.Tween.to.options.delay" name="ui.Tween.to.options.delay"></a>**`delay`** (integer, optional). How many ticks to wait before the move starts. The value holds still while it waits, and [`is_moving`](#ui.Tween.is_moving) reports true. `0` by default.

      Returns: [trx.ui.Tween](#ui.Tween). The same tween.

- <a id="ui.Widget" name="ui.Widget"></a>[lua]`trx.ui.Widget`

    A reusable UI element drawn over the game.

    A widget holds its own state. Every setting that holds a value, rather than
    a widget or a function, also takes a signal that holds the value. The widget
    remeasures only when a signal that it read while measuring changes, and
    when the room the interface lays out in changes. A [`trx.ui.Tween`](#ui.Tween) moves
    between ticks, so a widget reads one only while it paints, and reading one
    while it measures raises an error.

    A widget is in one place at a time: inside another widget, in a region or as
    the root of a layer. Using it in a second place is an error. Releasing a widget ends it and every widget under it, and
    using a released widget is an error.

    Methods:

    - <a id="ui.Widget.is_visible" name="ui.Widget.is_visible"></a>[lua]`widget:is_visible()`  
      Returns whether the widget participates in layout.

      A widget that is not visible keeps no room and leaves no gap.

      Returns: boolean. Whether it draws.

    - <a id="ui.Widget.measure" name="ui.Widget.measure"></a>[lua]`widget:measure()`  
      How much room the widget wants.

      Returns:
      - number. The width, in canvas units.
      - number. The height, in canvas units.

    - <a id="ui.Widget.paint" name="ui.Widget.paint"></a>[lua]`widget:paint(x, y, w, h)`  
      Draws the widget in an assigned box.

      [`trx.ui.regions.place`](#ui.regions.place) calls this automatically. Custom layout code can call
      it during [`trx.ui.on_paint`](#ui.on_paint).

      Parameters:
      - <a id="ui.Widget.paint.x" name="ui.Widget.paint.x"></a>**`x`** (number). The left edge.
      - <a id="ui.Widget.paint.y" name="ui.Widget.paint.y"></a>**`y`** (number). The top edge.
      - <a id="ui.Widget.paint.w" name="ui.Widget.paint.w"></a>**`w`** (number). The width it was given.
      - <a id="ui.Widget.paint.h" name="ui.Widget.paint.h"></a>**`h`** (number). The height it was given.

    - <a id="ui.Widget.release" name="ui.Widget.release"></a>[lua]`widget:release()`  
      Ends the widget and every widget under it: removes the widget from its
      region if it is placed, and detaches each one from its signals. Does nothing
      if the widget is already released.

      Signals keep references to their listeners. Release temporary widgets when
      they are no longer needed. A widget that another widget or a layer holds is
      released with its holder, and releasing it directly is an error. A widget that a level script placed is released when the level ends.

    - <a id="ui.Widget.wake" name="ui.Widget.wake"></a>[lua]`widget:wake()`  
      Invalidates the widget's cached size manually.

      Returns: [trx.ui.Widget](#ui.Widget). The same widget.

- <a id="ui.Carousel" name="ui.Carousel"></a>[lua]`trx.ui.Carousel`

    A row of items turned to the one in the middle.

    Methods:

    - <a id="ui.Carousel.is_sliding" name="ui.Carousel.is_sliding"></a>[lua]`carousel:is_sliding()`  
      Returns whether the row is still sliding to the selected item.

      Returns: boolean. Whether it slides.

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
      Reads the menu keys for one tick. Up and down move the cursor, confirm
      picks the row under it, and back reports that the player backed out. Uses up
      only the presses that it reads.

      Parameters:
      - <a id="ui.List.control.keys" name="ui.List.control.keys"></a>**`keys`** ([trx.ui.LayerKeys](#ui.LayerKeys)). The input of the layer the list is on.

      Returns:
      - integer or `nil`. The picked row, or `nil` where none was picked.
      - boolean. Whether the player backed out.

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

- <a id="ui.signals" name="ui.signals"></a>[lua]`trx.ui.signals`  
  What the interface tells a script about the room that it lays out in.

- <a id="ui.layers" name="ui.layers"></a>[lua]`trx.ui.layers`  
  Draws screens of widgets over the rest of the interface.

  A layer holds one widget tree, such as a menu or a question. Layers are kept
  in a stack. Each layer draws over the layers below it and over the widgets
  placed with [`trx.ui.regions.place`](#ui.regions.place). The engine interface still draws over
  all of them.

  Only the active layer, the top layer that reads input, reads the player's
  input. The layers below it read nothing until it closes. While a layer that
  reads input is open during play, the layers take the devices from the game
  with [`trx.input.grab`](INPUT.md#input.grab): Lara does not move, and the game's own keys, such as
  the pause key, do nothing. The level keeps running.

  A layer that a level script pushes closes when the level ends.

- <a id="ui.primitive" name="ui.primitive"></a>[lua]`trx.ui.primitive`  
  Low-level drawing calls and layout reservations.

  Use [`trx.ui.widgets`](#ui.widgets) for normal UI. Use these primitives only when building
  a custom widget. Primitive drawing does not affect region layout unless code
  reserves space first.

  Drawing calls are available only during [`trx.ui.on_paint`](#ui.on_paint). They
  report an error at any other time.

- <a id="ui.regions" name="ui.regions"></a>[lua]`trx.ui.regions`  
  Places script widgets on the screen.

  The screen has nine regions. The shipped scripts place the bars, the
  overlay text and the inventory ring hints in them, and dialogs keep clear
  of them. A widget placed in a region stacks with the others there, in the
  order that [`place.options`](#ui.regions.place.options) gives.

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

- <a id="ui.ease" name="ui.ease"></a>[lua]`trx.ui.ease`  
  Curves that shape how a [`trx.ui.Tween`](#ui.Tween) moves. Each takes how far the move
  has gone in time, from `0` to `1`, and returns how far the value has gone.

- <a id="ui.widgets" name="ui.widgets"></a>[lua]`trx.ui.widgets`  
  The widgets a script builds its screen from.

  A widget is created once and kept. Give it signals instead of fixed values,
  and it remeasures when they change.

  Put a widget on screen with [`trx.ui.regions.place`](#ui.regions.place).

- <a id="ui.on_reserve" name="ui.on_reserve"></a>[lua]`trx.ui.on_reserve(callback)`  
  Happens while the interface is built, once for each region. The handler
  reserves room in the region it is handed with [`trx.ui.primitive.reserve`](#ui.primitive.reserve),
  and paints into that room later during [`trx.ui.on_paint`](#ui.on_paint).

  It happens anywhere the game draws its interface, including fades, FMVs
  and normal play, and it follows the frame rate rather than the game clock.

  Parameters:
  - <a id="ui.on_reserve.callback" name="ui.on_reserve.callback"></a>**`callback`** (function). What to run for each region.
    Called with:
    - <a id="ui.on_reserve.region" name="ui.on_reserve.region"></a>**`region`** ([trx.ui.Region](#ui.Region)). The region being built.

  Returns: [trx.events.Listener](EVENTS.md#events.Listener). The attached handler.

  Example:
  ```lua
  local slot = nil
  
  trx.ui.on_reserve(function(region)
    if region == trx.ui.Region.TOP_CENTER then
      local w, h = trx.ui.primitive.measure_text("hello")
      slot = trx.ui.primitive.reserve(region, w, h)
    end
  end)
  
  trx.ui.on_paint(function()
    local x, y = trx.ui.primitive.slot_box(slot)
    if x ~= nil then
      trx.ui.primitive.text("hello", x, y)
    end
  end)
  ```

- <a id="ui.on_paint" name="ui.on_paint"></a>[lua]`trx.ui.on_paint(callback)`  
  Happens once the interface is laid out and before it is drawn, under the
  engine interface. The boxes reserved during [`trx.ui.on_reserve`](#ui.on_reserve) are known by
  then, and the primitive drawing calls work here and nowhere else. The
  console still draws over what is painted here.

  Parameters:
  - <a id="ui.on_paint.callback" name="ui.on_paint.callback"></a>**`callback`** (function). What to run when it happens.

  Returns: [trx.events.Listener](EVENTS.md#events.Listener). The attached handler.

- <a id="ui.safe_area_for" name="ui.safe_area_for"></a>[lua]`trx.ui.safe_area_for(width)`  
  The part of the canvas that a box of the given width, centered, is free to
  take. Unlike [`trx.ui.safe_area`](#ui.safe_area), an edge of the interface takes room from it
  only if the box is wide enough to reach that edge, so a narrow box ignores
  what sits in a corner, such as the FPS counter.

  Parameters:
  - <a id="ui.safe_area_for.width" name="ui.safe_area_for.width"></a>**`width`** (number). How wide the box is, in canvas units.

  Returns: [trx.ui.Area](#ui.Area). The room the box has.

  Example:
  ```lua
  local area = trx.ui.safe_area_for(200)
  ```

- <a id="ui.is_ps1_bar" name="ui.is_ps1_bar"></a>[lua]`trx.ui.is_ps1_bar(type)`  
  Whether a bar of the given type takes the PS1 look, which the player picks
  in the settings and which decides which of its colors applies.

  Parameters:
  - <a id="ui.is_ps1_bar.type" name="ui.is_ps1_bar.type"></a>**`type`** ([trx.ui.BarType](#ui.BarType)). The bar.

  Returns: boolean. True for the PS1 look.

- <a id="ui.interpolation_rate" name="ui.interpolation_rate"></a>[lua]`trx.ui.interpolation_rate()`  
  Returns the interpolation factor of the frame being drawn between the last
  two ticks: `0` at the earlier tick and `1` at the later one.
  [`trx.ui.Tween`](#ui.Tween) reads it to move smoothly between ticks.

  Returns: number. The factor, from `0` to `1`.

- <a id="ui.input_hint_text" name="ui.input_hint_text"></a>[lua]`trx.ui.input_hint_text(input, [words])`  
  The text that shows the keys bound to one or more input roles, as the game
  shows them, followed by some words. The keys follow the player's bindings
  and controller.

  Parameters:
  - <a id="ui.input_hint_text.input" name="ui.input_hint_text.input"></a>**`input`** ([trx.ui.Inputs](#ui.Inputs)). The role, or the roles shown side by side.
  - <a id="ui.input_hint_text.words" name="ui.input_hint_text.words"></a>**`words`** (string, optional). What the keys do.

  Returns: string. The text.

  Example:
  ```lua
  local text = trx.ui.input_hint_text(trx.input.Role.MENU_CONFIRM, "Use")
  ```

- <a id="ui.layers.push" name="ui.layers.push"></a>[lua]`trx.ui.layers.push(settings)`  
  Puts a layer on top of the stack.

  The layer reads no input on the tick it is pushed, because the press that
  opened it is often still active.

  Parameters:
  - <a id="ui.layers.push.settings" name="ui.layers.push.settings"></a>**`settings`** (table). The layer settings.

    Keys:
    - <a id="ui.layers.push.settings.root" name="ui.layers.push.settings.root"></a>**`root`** ([trx.ui.Widget](#ui.Widget)). The widget tree to draw.
    - <a id="ui.layers.push.settings.place" name="ui.layers.push.settings.place"></a>**`place`** ([trx.ui.Placement](#ui.Placement) or function, optional). Where the tree goes. A function receives the width and the height that the tree measures, and returns the top left corner of the tree, in canvas units. The layer works the corner out again only when the tree's size, the room the interface lays out in, or a signal that the placement reads changes, and on every frame while the placement reads a [`trx.ui.Tween`](#ui.Tween). The tree is centered in [`trx.ui.safe_area_for`](#ui.safe_area_for) its width by default.
    - <a id="ui.layers.push.settings.modal" name="ui.layers.push.settings.modal"></a>**`modal`** (boolean, optional). Whether the layer reads input. A layer that reads input takes it from the layers below, and during play from the game. By default, a layer reads input when it has `on_input`. Set it to `true` for a layer that holds the input back without reading any.
    - <a id="ui.layers.push.settings.on_input" name="ui.layers.push.settings.on_input"></a>**`on_input`** (function, optional). Runs once a tick while the layer is the active layer. It receives the layer and a [`trx.ui.LayerKeys`](#ui.LayerKeys). An error closes the layer.
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

- <a id="ui.layers.fade" name="ui.layers.fade"></a>[lua]`trx.ui.layers.fade(settings)`  
  Puts up a fade layer, on top of the others drawn where it is. A level
  script's layer closes when the level ends.

  Parameters:
  - <a id="ui.layers.fade.settings" name="ui.layers.fade.settings"></a>**`settings`** (table). The layer settings.

    Keys:
    - <a id="ui.layers.fade.settings.fill" name="ui.layers.fade.settings.fill"></a>**`fill`** ([trx.math.Color](MATH.md#math.Color), optional). The colour that the layer holds.
    - <a id="ui.layers.fade.settings.snapshot" name="ui.layers.fade.settings.snapshot"></a>**`snapshot`** (boolean, optional). Whether the layer holds the frame on screen now, in place of a colour.
    - <a id="ui.layers.fade.settings.backdrop" name="ui.layers.fade.settings.backdrop"></a>**`backdrop`** (string, optional). The backdrop style that the layer holds, in place of a colour, by the names that the `ui.stats_background_style` setting takes.
    - <a id="ui.layers.fade.settings.path" name="ui.layers.fade.settings.path"></a>**`path`** (string, optional). The picture of an `image` backdrop, as [`trx.ui.primitive.background`](#ui.primitive.background) takes it.
    - <a id="ui.layers.fade.settings.opacity" name="ui.layers.fade.settings.opacity"></a>**`opacity`** (number, optional). How opaque the layer starts, from `0` to `1`. `1` by default.
    - <a id="ui.layers.fade.settings.on_close" name="ui.layers.fade.settings.on_close"></a>**`on_close`** (function, optional). Runs once when the layer closes, for any reason. It receives the layer.
    - <a id="ui.layers.fade.settings.holds_screen" name="ui.layers.fade.settings.holds_screen"></a>**`holds_screen`** (boolean, optional). Whether the engine waits to start its next screen, or to exit, while the layer is open. `false` by default.
    - <a id="ui.layers.fade.settings.over_console" name="ui.layers.fade.settings.over_console"></a>**`over_console`** (boolean, optional). Whether the layer is drawn over the console too, as the fade to black as the game exits is. `false` by default.

  Returns: [trx.ui.FadeLayer](#ui.FadeLayer). The layer.

  Example:
  ```lua
  local black = trx.ui.layers.fade({ fill = trx.math.color("000000"), opacity = 0 })
  black:to(1, 15, { close = true })
  ```

- <a id="ui.layers.active" name="ui.layers.active"></a>[lua]`trx.ui.layers.active()`  
  Returns the active layer: the top layer that reads input.

  Returns: [trx.ui.StackLayer](#ui.StackLayer) or `nil`. The layer, or `nil` if no layer reads input.

- <a id="ui.layers.count" name="ui.layers.count"></a>[lua]`trx.ui.layers.count()`  
  Returns how many layers are on the stack.

  Returns: integer. The number of layers.

- <a id="ui.primitive.reserve" name="ui.primitive.reserve"></a>[lua]`trx.ui.primitive.reserve(region, w, h)`  
  Reserves space in a region and returns a slot for it.

  The reservation is stacked with the engine UI in that region. Reserve space
  during [`trx.ui.on_reserve`](#ui.on_reserve), then read the assigned box during
  [`trx.ui.on_paint`](#ui.on_paint).

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
  - number or `nil`. The left edge, or `nil` when the slot is no longer valid.
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

- <a id="ui.primitive.wrap_text" name="ui.primitive.wrap_text"></a>[lua]`trx.ui.primitive.wrap_text(text, width, [scale])`  
  Breaks text into lines that fit a width, the way the game wraps its own
  text. Available at any time.

  Parameters:
  - <a id="ui.primitive.wrap_text.text" name="ui.primitive.wrap_text.text"></a>**`text`** (string). What to wrap.
  - <a id="ui.primitive.wrap_text.width" name="ui.primitive.wrap_text.width"></a>**`width`** (number). The widest a line may be, in canvas units.
  - <a id="ui.primitive.wrap_text.scale" name="ui.primitive.wrap_text.scale"></a>**`scale`** (number, optional). Multiplies the text size. `1.0` by default.

  Returns: string. The text with line breaks where the lines end.

  Example:
  ```lua
  local lines = trx.ui.primitive.wrap_text(description, 300)
  ```

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
  - <a id="ui.primitive.quad.color" name="ui.primitive.quad.color"></a>**`color`** ([trx.math.Color](MATH.md#math.Color) or table). What color to fill it with: a [`trx.math.Color`](MATH.md#math.Color), or a table of `r`, `g`, `b` and `a` channels from 0 to 255. The color is opaque where `a` is absent.

- <a id="ui.primitive.gradient_quad" name="ui.primitive.gradient_quad"></a>[lua]`trx.ui.primitive.gradient_quad(x, y, z, w, h, tl, tr, bl, br)`  
  Draws a rectangle whose corners each carry a color.

  Parameters:
  - <a id="ui.primitive.gradient_quad.x" name="ui.primitive.gradient_quad.x"></a>**`x`** (number). The left edge.
  - <a id="ui.primitive.gradient_quad.y" name="ui.primitive.gradient_quad.y"></a>**`y`** (number). The top edge.
  - <a id="ui.primitive.gradient_quad.z" name="ui.primitive.gradient_quad.z"></a>**`z`** (integer). What to draw in front of.
  - <a id="ui.primitive.gradient_quad.w" name="ui.primitive.gradient_quad.w"></a>**`w`** (number). The width.
  - <a id="ui.primitive.gradient_quad.h" name="ui.primitive.gradient_quad.h"></a>**`h`** (number). The height.
  - <a id="ui.primitive.gradient_quad.tl" name="ui.primitive.gradient_quad.tl"></a>**`tl`** ([trx.math.Color](MATH.md#math.Color) or table). The top-left color, in either form that [`quad`](#ui.primitive.quad) takes.
  - <a id="ui.primitive.gradient_quad.tr" name="ui.primitive.gradient_quad.tr"></a>**`tr`** ([trx.math.Color](MATH.md#math.Color) or table). The top-right color, in either form that [`quad`](#ui.primitive.quad) takes.
  - <a id="ui.primitive.gradient_quad.bl" name="ui.primitive.gradient_quad.bl"></a>**`bl`** ([trx.math.Color](MATH.md#math.Color) or table). The bottom-left color, in either form that [`quad`](#ui.primitive.quad) takes.
  - <a id="ui.primitive.gradient_quad.br" name="ui.primitive.gradient_quad.br"></a>**`br`** ([trx.math.Color](MATH.md#math.Color) or table). The bottom-right color, in either form that [`quad`](#ui.primitive.quad) takes.

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

- <a id="ui.primitive.model" name="ui.primitive.model"></a>[lua]`trx.ui.primitive.model(object, x, y, w, h, [opts])`  
  Draws an object's model in a box on the canvas, fitted into the box. The
  model stands where it is painted among the rest of the interface: over what
  was painted before it, under what is painted after.

  Draws nothing where the level did not load the object, or where the object
  carries no model.

  Parameters:
  - <a id="ui.primitive.model.object" name="ui.primitive.model.object"></a>**`object`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The object to draw.
  - <a id="ui.primitive.model.x" name="ui.primitive.model.x"></a>**`x`** (number). The left edge.
  - <a id="ui.primitive.model.y" name="ui.primitive.model.y"></a>**`y`** (number). The top edge.
  - <a id="ui.primitive.model.w" name="ui.primitive.model.w"></a>**`w`** (number). The width of the box.
  - <a id="ui.primitive.model.h" name="ui.primitive.model.h"></a>**`h`** (number). The height of the box.
  - <a id="ui.primitive.model.opts" name="ui.primitive.model.opts"></a>**`opts`** (table, optional). How to turn it, which meshes to draw and how to light them.

    Keys:
    - <a id="ui.primitive.model.opts.rot" name="ui.primitive.model.opts.rot"></a>**`rot`** ([trx.math.Rot](MATH.md#math.Rot), optional). How far to turn the model. Not turned by default.
    - <a id="ui.primitive.model.opts.mesh_mask" name="ui.primitive.model.opts.mesh_mask"></a>**`mesh_mask`** (integer, optional). Which of its meshes to draw, one bit each. Every mesh by default.
    - <a id="ui.primitive.model.opts.light" name="ui.primitive.model.opts.light"></a>**`light`** (table, optional). The light to draw it in. The light the game holds up a pickup in by default.
    - <a id="ui.primitive.model.opts.held" name="ui.primitive.model.opts.held"></a>**`held`** (boolean, optional). Whether to tilt and turn it first as the game holds up a pickup, which shows its face. `true` by default; `false` draws it as it was built.
    - <a id="ui.primitive.model.opts.tilt" name="ui.primitive.model.opts.tilt"></a>**`tilt`** ([trx.math.Angle](MATH.md#math.Angle), optional). How far to tilt it towards the viewer before anything else, about the screen's horizontal axis, as a camera looking down on it would. Not tilted by default.

  Example:
  ```lua
  trx.ui.primitive.model(trx.catalog.objects.small_medipack_option,
    100, 50, 64, 48, { rot = { x = 0, y = 30 * trx.math.DEG_1, z = 0 } })
  ```

- <a id="ui.primitive.world_model" name="ui.primitive.world_model"></a>[lua]`trx.ui.primitive.world_model(object, opts)`  
  Draws an object's model where it stands in a world of its own, as that
  world's camera sees it over the whole canvas: placed by moves and turns
  applied in order, the way the engine places a model, so that a script can
  show a model as the game would have shown it. It stands behind the whole
  interface, over what went behind it before, such as a background.

  Draws nothing where the level did not load the object, or where the object
  carries no model.

  Parameters:
  - <a id="ui.primitive.world_model.object" name="ui.primitive.world_model.object"></a>**`object`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The object to draw.
  - <a id="ui.primitive.world_model.opts" name="ui.primitive.world_model.opts"></a>**`opts`** (table). Where it stands, the camera that sees it, and its light.

    Keys:
    - <a id="ui.primitive.world_model.opts.camera" name="ui.primitive.world_model.opts.camera"></a>**`camera`** (table). The camera that sees the model.
    - <a id="ui.primitive.world_model.opts.pose" name="ui.primitive.world_model.opts.pose"></a>**`pose`** (a list of table). The steps that place the model, applied in order from the world's origin, up to sixteen.
    - <a id="ui.primitive.world_model.opts.mesh_mask" name="ui.primitive.world_model.opts.mesh_mask"></a>**`mesh_mask`** (integer, optional). Which of its meshes to draw, one bit each. Every mesh by default.
    - <a id="ui.primitive.world_model.opts.light" name="ui.primitive.world_model.opts.light"></a>**`light`** (table, optional). The light to draw it in, with its directions in the model's world. A plain white light by default.

- <a id="ui.primitive.darken" name="ui.primitive.darken"></a>[lua]`trx.ui.primitive.darken(opacity)`  
  Darkens the screen behind the whole interface, edge to edge, as the game
  darkens it behind a menu and fades it to black. It goes over what went
  behind the interface before it, such as a background or a model in a world
  of its own, and under the whole interface.

  Parameters:
  - <a id="ui.primitive.darken.opacity" name="ui.primitive.darken.opacity"></a>**`opacity`** (number). How dark, from `0` for not at all to `1` for black.

- <a id="ui.primitive.background" name="ui.primitive.background"></a>[lua]`trx.ui.primitive.background([path], [mono], [dim])`  
  Draws an image behind the whole interface, as the game draws the pictures
  behind its screens, fitted to the screen's shape and darkened by the dim.
  Without an image, or where it does not exist, the screen behind the
  interface is black. The game's scene behind the canvas is drawn over.

  Parameters:
  - <a id="ui.primitive.background.path" name="ui.primitive.background.path"></a>**`path`** (string, optional). The image file, named from the images directory, or as a screen reports it.
  - <a id="ui.primitive.background.mono" name="ui.primitive.background.mono"></a>**`mono`** (boolean, optional). Whether to draw it without its colour.
  - <a id="ui.primitive.background.dim" name="ui.primitive.background.dim"></a>**`dim`** (number, optional). How much to darken it, from `0` for not at all to `1` for black. Not darkened by default.

  Returns: boolean. Whether the image exists.

  Example:
  ```lua
  trx.ui.primitive.background("india.webp", true, 0.5)
  ```

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
  - <a id="ui.primitive.sprite.color" name="ui.primitive.sprite.color"></a>**`color`** ([trx.math.Color](MATH.md#math.Color) or table). What color to tint it with, in either form that [`quad`](#ui.primitive.quad) takes.

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
  - <a id="ui.primitive.gradient_sprite.tl" name="ui.primitive.gradient_sprite.tl"></a>**`tl`** ([trx.math.Color](MATH.md#math.Color) or table). The top-left color, in either form that [`quad`](#ui.primitive.quad) takes.
  - <a id="ui.primitive.gradient_sprite.tr" name="ui.primitive.gradient_sprite.tr"></a>**`tr`** ([trx.math.Color](MATH.md#math.Color) or table). The top-right color, in either form that [`quad`](#ui.primitive.quad) takes.
  - <a id="ui.primitive.gradient_sprite.bl" name="ui.primitive.gradient_sprite.bl"></a>**`bl`** ([trx.math.Color](MATH.md#math.Color) or table). The bottom-left color, in either form that [`quad`](#ui.primitive.quad) takes.
  - <a id="ui.primitive.gradient_sprite.br" name="ui.primitive.gradient_sprite.br"></a>**`br`** ([trx.math.Color](MATH.md#math.Color) or table). The bottom-right color, in either form that [`quad`](#ui.primitive.quad) takes.

- <a id="ui.regions.place" name="ui.regions.place"></a>[lua]`trx.ui.regions.place(region, widget, [options])`  
  Places a widget in a region.

  If the region argument is a signal, the widget moves when the signal
  changes. Placing a widget that is already placed moves it.

  Parameters:
  - <a id="ui.regions.place.region" name="ui.regions.place.region"></a>**`region`** (any). The target region, or a signal that holds one.
  - <a id="ui.regions.place.widget" name="ui.regions.place.widget"></a>**`widget`** ([trx.ui.Widget](#ui.Widget)). The widget to place.
  - <a id="ui.regions.place.options" name="ui.regions.place.options"></a>**`options`** ([trx.ui.regions.place.options](#ui.regions.place.options), optional). How it stands among the others.

  Example:
  ```lua
  trx.ui.regions.place(trx.ui.Region.TOP_LEFT, health_bar)
  ```

- <a id="ui.regions.remove" name="ui.regions.remove"></a>[lua]`trx.ui.regions.remove(widget)`  
  Takes a widget out of its region and keeps it, so that it can be placed
  again.

  To be done with a widget, release it with [`trx.ui.Widget:release`](#ui.Widget.release) instead,
  which takes it out of its region as well. Widgets that a level script placed
  are released when the level ends.

  Parameters:
  - <a id="ui.regions.remove.widget" name="ui.regions.remove.widget"></a>**`widget`** ([trx.ui.Widget](#ui.Widget)). The widget to remove.

  Returns: boolean. Whether the widget was in a region.

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

- <a id="ui.ease.linear" name="ui.ease.linear"></a>[lua]`trx.ui.ease.linear(ratio)`  
  The same pace throughout.

  Parameters:
  - <a id="ui.ease.linear.ratio" name="ui.ease.linear.ratio"></a>**`ratio`** (number). How far the move has gone in time.

  Returns: number. How far the value has gone.

- <a id="ui.ease.quad_in" name="ui.ease.quad_in"></a>[lua]`trx.ui.ease.quad_in(ratio)`  
  Slow to start.

  Parameters:
  - <a id="ui.ease.quad_in.ratio" name="ui.ease.quad_in.ratio"></a>**`ratio`** (number). How far the move has gone in time.

  Returns: number. How far the value has gone.

- <a id="ui.ease.quad_out" name="ui.ease.quad_out"></a>[lua]`trx.ui.ease.quad_out(ratio)`  
  Slow to finish.

  Parameters:
  - <a id="ui.ease.quad_out.ratio" name="ui.ease.quad_out.ratio"></a>**`ratio`** (number). How far the move has gone in time.

  Returns: number. How far the value has gone.

- <a id="ui.ease.quad_in_out" name="ui.ease.quad_in_out"></a>[lua]`trx.ui.ease.quad_in_out(ratio)`  
  Slow at both ends, fastest in the middle.

  Parameters:
  - <a id="ui.ease.quad_in_out.ratio" name="ui.ease.quad_in_out.ratio"></a>**`ratio`** (number). How far the move has gone in time.

  Returns: number. How far the value has gone.

- <a id="ui.tween" name="ui.tween"></a>[lua]`trx.ui.tween([value])`  
  Creates a tween that rests at a value.

  Parameters:
  - <a id="ui.tween.value" name="ui.tween.value"></a>**`value`** (number, optional). Where it rests. `0` by default.

  Returns: [trx.ui.Tween](#ui.Tween). The tween.

- <a id="ui.widgets.Bar" name="ui.widgets.Bar"></a>[lua]`trx.ui.widgets.Bar(settings)`  
  One of the game's bars, drawn with the player's bar settings.

  The bar uses the same theme, border, and fill bands as the engine UI. Use a
  signal for a fill value that changes.

  Parameters:
  - <a id="ui.widgets.Bar.settings" name="ui.widgets.Bar.settings"></a>**`settings`** (table). The bar settings.

    Keys:
    - <a id="ui.widgets.Bar.settings.type" name="ui.widgets.Bar.settings.type"></a>**`type`** (any). The [`trx.ui.BarType`](#ui.BarType) theme to use, or a signal that holds it.
    - <a id="ui.widgets.Bar.settings.value" name="ui.widgets.Bar.settings.value"></a>**`value`** (any). The fill amount from 0 to 1, or a signal that holds it.
    - <a id="ui.widgets.Bar.settings.w" name="ui.widgets.Bar.settings.w"></a>**`w`** (any, optional). The width, in canvas units at the default bar size, or a signal that holds it. The size its look gives it by default.
    - <a id="ui.widgets.Bar.settings.h" name="ui.widgets.Bar.settings.h"></a>**`h`** (any, optional). The height, in canvas units at the default bar size, or a signal that holds it. The size its look gives it by default.
    - <a id="ui.widgets.Bar.settings.smooth" name="ui.widgets.Bar.settings.smooth"></a>**`smooth`** (any, optional). Whether the fill shades one band into the next, or a signal that holds that value. The player's smooth bars setting by default.
    - <a id="ui.widgets.Bar.settings.preview" name="ui.widgets.Bar.settings.preview"></a>**`preview`** (any, optional). Whether the bar takes the size of the text around it rather than the player's bar size and the size its style gives it, as a bar shown beside its setting does, or a signal that holds that value. `false` by default.

  Returns: [trx.ui.Widget](#ui.Widget). The bar.

- <a id="ui.widgets.Carousel" name="ui.widgets.Carousel"></a>[lua]`trx.ui.widgets.Carousel(settings)`  
  A row of items turned to the one in the middle, as the inventory ring
  offers what to combine an entry with. The item in the middle is drawn at its
  own size, those beside it smaller, and the rest out of sight. Setting the
  selected signal slides the row there, the short way round.

  Parameters:
  - <a id="ui.widgets.Carousel.settings" name="ui.widgets.Carousel.settings"></a>**`settings`** (table). The carousel settings.

    Keys:
    - <a id="ui.widgets.Carousel.settings.items" name="ui.widgets.Carousel.settings.items"></a>**`items`** (a list of any). What the row holds, in order.
    - <a id="ui.widgets.Carousel.settings.selected" name="ui.widgets.Carousel.settings.selected"></a>**`selected`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). The place in the row, counted from `1`, of the item in the middle. Setting it slides the row there.
    - <a id="ui.widgets.Carousel.settings.build" name="ui.widgets.Carousel.settings.build"></a>**`build`** (function). Makes the widget for an item. It receives the item and its place in the row, and returns a widget.
    - <a id="ui.widgets.Carousel.settings.w" name="ui.widgets.Carousel.settings.w"></a>**`w`** (any). The width of the row, in canvas units at the default text size, or a signal that holds it.
    - <a id="ui.widgets.Carousel.settings.h" name="ui.widgets.Carousel.settings.h"></a>**`h`** (any). The height of the row, in canvas units at the default text size, or a signal that holds it.
    - <a id="ui.widgets.Carousel.settings.spacing" name="ui.widgets.Carousel.settings.spacing"></a>**`spacing`** (any). How far apart the middles of two neighbours stand, in canvas units at the default text size, or a signal that holds it.
    - <a id="ui.widgets.Carousel.settings.side_size" name="ui.widgets.Carousel.settings.side_size"></a>**`side_size`** (any, optional). How large the items beside the middle are drawn, as a share of their own size, or a signal that holds it. `0.6` by default.
    - <a id="ui.widgets.Carousel.settings.ticks" name="ui.widgets.Carousel.settings.ticks"></a>**`ticks`** (any, optional). How many ticks a slide takes, or a signal that holds it. `8` by default.
    - <a id="ui.widgets.Carousel.settings.curve" name="ui.widgets.Carousel.settings.curve"></a>**`curve`** (any, optional). The curve of a slide, one of [`trx.ui.ease`](#ui.ease), or a signal that holds it. [`trx.ui.ease.quad_in_out`](#ui.ease.quad_in_out) by default.
    - <a id="ui.widgets.Carousel.settings.visible" name="ui.widgets.Carousel.settings.visible"></a>**`visible`** (any, optional). Whether the widget is visible, or a signal that holds that value. A widget that is not visible keeps no room and leaves no gap. [`Blank`](#ui.widgets.Blank) keeps the room instead.

  Returns: [trx.ui.Carousel](#ui.Carousel). The carousel.

  Example:
  ```lua
  local selected = trx.signal.new(1)
  trx.ui.widgets.Carousel({
    items = objects,
    selected = selected,
    build = function(object)
      return trx.ui.widgets.Model({ object = object, w = 100, h = 120 })
    end,
    w = 260,
    h = 150,
    spacing = 90,
  })
  ```

- <a id="ui.widgets.Custom" name="ui.widgets.Custom"></a>[lua]`trx.ui.widgets.Custom(settings)`  
  A widget that measures and draws itself through functions the script gives.

  Use it for drawing that the other widgets do not cover. The widget
  remeasures when a signal that `measure` read changes.

  Parameters:
  - <a id="ui.widgets.Custom.settings" name="ui.widgets.Custom.settings"></a>**`settings`** (table). The widget settings.

    Keys:
    - <a id="ui.widgets.Custom.settings.measure" name="ui.widgets.Custom.settings.measure"></a>**`measure`** (function). Returns the width and the height the widget wants, in canvas units.
    - <a id="ui.widgets.Custom.settings.paint" name="ui.widgets.Custom.settings.paint"></a>**`paint`** (function). Draws the widget with [`trx.ui.primitive`](#ui.primitive). It receives the left edge, the top edge, the width and the height of the box the widget was given.
    - <a id="ui.widgets.Custom.settings.visible" name="ui.widgets.Custom.settings.visible"></a>**`visible`** (any, optional). Whether the widget is visible, or a signal that holds that value. A widget that is not visible keeps no room and leaves no gap. [`Blank`](#ui.widgets.Blank) keeps the room instead.

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
    - <a id="ui.widgets.Digits.settings.object" name="ui.widgets.Digits.settings.object"></a>**`object`** (any). The sprite object to draw the characters from, or a signal carrying it.
    - <a id="ui.widgets.Digits.settings.text" name="ui.widgets.Digits.settings.text"></a>**`text`** (any). The text, or a signal carrying it.
    - <a id="ui.widgets.Digits.settings.color" name="ui.widgets.Digits.settings.color"></a>**`color`** (any). What color to draw the characters in, or a signal carrying one.
    - <a id="ui.widgets.Digits.settings.color_bottom" name="ui.widgets.Digits.settings.color_bottom"></a>**`color_bottom`** (any, optional). The color the characters fade to down their height. The main color by default, which draws them flat.
    - <a id="ui.widgets.Digits.settings.mark_color" name="ui.widgets.Digits.settings.mark_color"></a>**`mark_color`** (any, optional). What color to draw the `T` in. The main color by default.
    - <a id="ui.widgets.Digits.settings.mark_color_bottom" name="ui.widgets.Digits.settings.mark_color_bottom"></a>**`mark_color_bottom`** (any, optional). The color the `T` fades to. Its own color by default.
    - <a id="ui.widgets.Digits.settings.visible" name="ui.widgets.Digits.settings.visible"></a>**`visible`** (any, optional). Whether the widget is visible, or a signal that holds that value. A widget that is not visible keeps no room and leaves no gap. [`Blank`](#ui.widgets.Blank) keeps the room instead.

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
    - <a id="ui.widgets.Image.settings.w" name="ui.widgets.Image.settings.w"></a>**`w`** (any). The width, in canvas units, or a signal that holds it.
    - <a id="ui.widgets.Image.settings.h" name="ui.widgets.Image.settings.h"></a>**`h`** (any). The height, in canvas units, or a signal that holds it.
    - <a id="ui.widgets.Image.settings.opacity" name="ui.widgets.Image.settings.opacity"></a>**`opacity`** (any, optional). How solid the image is, from 0 to 1, or a signal that holds that value. `1` by default.
    - <a id="ui.widgets.Image.settings.visible" name="ui.widgets.Image.settings.visible"></a>**`visible`** (any, optional). Whether the widget is visible, or a signal that holds that value. A widget that is not visible keeps no room and leaves no gap. [`Blank`](#ui.widgets.Blank) keeps the room instead.

  Returns: [trx.ui.Widget](#ui.Widget). The image.

- <a id="ui.widgets.InputHint" name="ui.widgets.InputHint"></a>[lua]`trx.ui.widgets.InputHint(settings)`  
  The keys bound to an input role, as the game shows them, and what they do.
  The keys follow the player's bindings and controller.

  Parameters:
  - <a id="ui.widgets.InputHint.settings" name="ui.widgets.InputHint.settings"></a>**`settings`** (table). The hint settings.

    Keys:
    - <a id="ui.widgets.InputHint.settings.input" name="ui.widgets.InputHint.settings.input"></a>**`input`** (any). The input role whose keys to show, a list of roles shown side by side, or a signal carrying either.
    - <a id="ui.widgets.InputHint.settings.text" name="ui.widgets.InputHint.settings.text"></a>**`text`** (any, optional). What the keys do, or a signal carrying it.
    - <a id="ui.widgets.InputHint.settings.scale" name="ui.widgets.InputHint.settings.scale"></a>**`scale`** (any, optional). Multiplies the text size, or a signal that holds it. `1.0` by default.
    - <a id="ui.widgets.InputHint.settings.visible" name="ui.widgets.InputHint.settings.visible"></a>**`visible`** (any, optional). Whether the widget is visible, or a signal that holds that value. A widget that is not visible keeps no room and leaves no gap. [`Blank`](#ui.widgets.Blank) keeps the room instead.

  Returns: [trx.ui.Widget](#ui.Widget). The hint.

  Example:
  ```lua
  trx.ui.widgets.InputHint({
    input = trx.input.Role.MENU_BACK,
    text = trx.locale.get("general/actions/cancel"),
  })
  ```

- <a id="ui.widgets.Label" name="ui.widgets.Label"></a>[lua]`trx.ui.widgets.Label(settings)`  
  A line of text. Use a signal for text that changes.

  Parameters:
  - <a id="ui.widgets.Label.settings" name="ui.widgets.Label.settings"></a>**`settings`** (table). The label settings.

    Keys:
    - <a id="ui.widgets.Label.settings.text" name="ui.widgets.Label.settings.text"></a>**`text`** (any). The text, or a signal carrying it.
    - <a id="ui.widgets.Label.settings.scale" name="ui.widgets.Label.settings.scale"></a>**`scale`** (any, optional). Multiplies the text size, or a signal that holds it. `1.0` by default.
    - <a id="ui.widgets.Label.settings.visible" name="ui.widgets.Label.settings.visible"></a>**`visible`** (any, optional). Whether the widget is visible, or a signal that holds that value. A widget that is not visible keeps no room and leaves no gap. [`Blank`](#ui.widgets.Blank) keeps the room instead.

  Returns: [trx.ui.Widget](#ui.Widget). The label.

- <a id="ui.widgets.Resize" name="ui.widgets.Resize"></a>[lua]`trx.ui.widgets.Resize(settings)`  
  Gives a child widget an explicit size.

  Parameters:
  - <a id="ui.widgets.Resize.settings" name="ui.widgets.Resize.settings"></a>**`settings`** (table). The resize settings.

    Keys:
    - <a id="ui.widgets.Resize.settings.child" name="ui.widgets.Resize.settings.child"></a>**`child`** ([trx.ui.Widget](#ui.Widget)). The child widget.
    - <a id="ui.widgets.Resize.settings.w" name="ui.widgets.Resize.settings.w"></a>**`w`** (any, optional). The width, in canvas units, or a signal that holds it. Its own by default.
    - <a id="ui.widgets.Resize.settings.h" name="ui.widgets.Resize.settings.h"></a>**`h`** (any, optional). The height, in canvas units, or a signal that holds it. Its own by default.
    - <a id="ui.widgets.Resize.settings.scaled" name="ui.widgets.Resize.settings.scaled"></a>**`scaled`** (any, optional). Whether `w` and `h` are at the default text size, and follow the text scale as [`Spacer`](#ui.widgets.Spacer) does, or a signal that holds that value. `false` by default.
    - <a id="ui.widgets.Resize.settings.visible" name="ui.widgets.Resize.settings.visible"></a>**`visible`** (any, optional). Whether the widget is visible, or a signal that holds that value. A widget that is not visible keeps no room and leaves no gap. [`Blank`](#ui.widgets.Blank) keeps the room instead.

  Returns: [trx.ui.Widget](#ui.Widget). The resized widget.

- <a id="ui.widgets.Blank" name="ui.widgets.Blank"></a>[lua]`trx.ui.widgets.Blank(settings)`  
  Leaves a child widget blank while a condition holds: it keeps its room, so
  nothing around it moves, and draws nothing.

  Parameters:
  - <a id="ui.widgets.Blank.settings" name="ui.widgets.Blank.settings"></a>**`settings`** (table). The settings.

    Keys:
    - <a id="ui.widgets.Blank.settings.child" name="ui.widgets.Blank.settings.child"></a>**`child`** ([trx.ui.Widget](#ui.Widget)). The child widget.
    - <a id="ui.widgets.Blank.settings.when" name="ui.widgets.Blank.settings.when"></a>**`when`** (any). Whether to leave the child blank, or a signal that holds that value.
    - <a id="ui.widgets.Blank.settings.visible" name="ui.widgets.Blank.settings.visible"></a>**`visible`** (any, optional). Whether the widget is visible, or a signal that holds that value. A widget that is not visible keeps no room and leaves no gap. [`Blank`](#ui.widgets.Blank) keeps the room instead.

  Returns: [trx.ui.Widget](#ui.Widget). The widget.

  Example:
  ```lua
  trx.ui.widgets.Blank({ when = ~can_step_left, child = left_arrow })
  ```

- <a id="ui.widgets.Spacer" name="ui.widgets.Spacer"></a>[lua]`trx.ui.widgets.Spacer([settings])`  
  Keeps empty room, which follows the text scale the same way the widgets
  beside it do. With no size, it takes no room, which suits a layer that shows
  nothing yet.

  Parameters:
  - <a id="ui.widgets.Spacer.settings" name="ui.widgets.Spacer.settings"></a>**`settings`** (table, optional). The spacer settings.

    Keys:
    - <a id="ui.widgets.Spacer.settings.w" name="ui.widgets.Spacer.settings.w"></a>**`w`** (any, optional). The width, in canvas units at the default text size, or a signal that holds it. `0` by default.
    - <a id="ui.widgets.Spacer.settings.h" name="ui.widgets.Spacer.settings.h"></a>**`h`** (any, optional). The height, in canvas units at the default text size, or a signal that holds it. `0` by default.
    - <a id="ui.widgets.Spacer.settings.visible" name="ui.widgets.Spacer.settings.visible"></a>**`visible`** (any, optional). Whether the widget is visible, or a signal that holds that value. A widget that is not visible keeps no room and leaves no gap. [`Blank`](#ui.widgets.Blank) keeps the room instead.

  Returns: [trx.ui.Widget](#ui.Widget). The spacer.

  Example:
  ```lua
  trx.ui.widgets.Stack({
    children = { title, trx.ui.widgets.Spacer({ h = 5 }), body },
  })
  ```

- <a id="ui.widgets.Pad" name="ui.widgets.Pad"></a>[lua]`trx.ui.widgets.Pad(settings)`  
  Keeps a margin around a child widget.

  The margin is in canvas units at the default text size, and follows the text
  scale the same way the widgets inside it do.

  Parameters:
  - <a id="ui.widgets.Pad.settings" name="ui.widgets.Pad.settings"></a>**`settings`** (table). The padding settings.

    Keys:
    - <a id="ui.widgets.Pad.settings.child" name="ui.widgets.Pad.settings.child"></a>**`child`** ([trx.ui.Widget](#ui.Widget)). The child widget.
    - <a id="ui.widgets.Pad.settings.x" name="ui.widgets.Pad.settings.x"></a>**`x`** (any, optional). The margin at the left and the right, or a signal that holds it. `0` by default.
    - <a id="ui.widgets.Pad.settings.y" name="ui.widgets.Pad.settings.y"></a>**`y`** (any, optional). The margin at the top and the bottom, or a signal that holds it. `0` by default.
    - <a id="ui.widgets.Pad.settings.visible" name="ui.widgets.Pad.settings.visible"></a>**`visible`** (any, optional). Whether the widget is visible, or a signal that holds that value. A widget that is not visible keeps no room and leaves no gap. [`Blank`](#ui.widgets.Blank) keeps the room instead.

  Returns: [trx.ui.Widget](#ui.Widget). The padded widget.

- <a id="ui.widgets.Frame" name="ui.widgets.Frame"></a>[lua]`trx.ui.widgets.Frame(settings)`  
  Draws one of the game's frames behind a child widget.

  The frame takes the whole box the child asks for, so pad the child where the
  text would otherwise sit against the edge.

  Parameters:
  - <a id="ui.widgets.Frame.settings" name="ui.widgets.Frame.settings"></a>**`settings`** (table). The frame settings.

    Keys:
    - <a id="ui.widgets.Frame.settings.child" name="ui.widgets.Frame.settings.child"></a>**`child`** ([trx.ui.Widget](#ui.Widget)). The child widget.
    - <a id="ui.widgets.Frame.settings.style" name="ui.widgets.Frame.settings.style"></a>**`style`** (any, optional). Which [`trx.ui.FrameStyle`](#ui.FrameStyle) to draw, or a signal that holds it. `false` draws no frame, as for a row that the cursor is not on. The dialog box by default.
    - <a id="ui.widgets.Frame.settings.z" name="ui.widgets.Frame.settings.z"></a>**`z`** (any, optional). The draw order, or a signal that holds it. `160` by default, which is behind text.
    - <a id="ui.widgets.Frame.settings.visible" name="ui.widgets.Frame.settings.visible"></a>**`visible`** (any, optional). Whether the widget is visible, or a signal that holds that value. A widget that is not visible keeps no room and leaves no gap. [`Blank`](#ui.widgets.Blank) keeps the room instead.

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
    - <a id="ui.widgets.Fit.settings.visible" name="ui.widgets.Fit.settings.visible"></a>**`visible`** (any, optional). Whether the widget is visible, or a signal that holds that value. A widget that is not visible keeps no room and leaves no gap. [`Blank`](#ui.widgets.Blank) keeps the room instead.

  Returns: [trx.ui.Widget](#ui.Widget). The fitted widget.

- <a id="ui.widgets.Stack" name="ui.widgets.Stack"></a>[lua]`trx.ui.widgets.Stack(settings)`  
  Lays widgets out one after another.

  Widgets that are not shown take no room and leave no gap.

  Parameters:
  - <a id="ui.widgets.Stack.settings" name="ui.widgets.Stack.settings"></a>**`settings`** (table). The stack settings.

    Keys:
    - <a id="ui.widgets.Stack.settings.children" name="ui.widgets.Stack.settings.children"></a>**`children`** (a list of [trx.ui.Widget](#ui.Widget)). The widgets, in the order they are laid out.
    - <a id="ui.widgets.Stack.settings.orientation" name="ui.widgets.Stack.settings.orientation"></a>**`orientation`** (any, optional). The [`trx.ui.Orientation`](#ui.Orientation) to lay out in, or a signal that holds it. Vertical by default.
    - <a id="ui.widgets.Stack.settings.spacing" name="ui.widgets.Stack.settings.spacing"></a>**`spacing`** (any, optional). The gap between one and the next, in canvas units at the default text size, or a signal that holds it. `0` by default.
    - <a id="ui.widgets.Stack.settings.align" name="ui.widgets.Stack.settings.align"></a>**`align`** (any, optional). The [`trx.ui.HAlign`](#ui.HAlign), or a signal that holds it, that says where the children sit across the width. In a vertical stack, it places each narrower child. In a horizontal stack, it places the whole row, and [`trx.ui.HAlign.DISTRIBUTE`](#ui.HAlign) spreads the children out. Left by default.
    - <a id="ui.widgets.Stack.settings.v_align" name="ui.widgets.Stack.settings.v_align"></a>**`v_align`** (any, optional). The [`trx.ui.VAlign`](#ui.VAlign), or a signal that holds it, that says where the children sit down the height. In a horizontal stack, it places each shorter child. In a vertical stack, it places the whole column, and [`trx.ui.VAlign.DISTRIBUTE`](#ui.VAlign) spreads the children out. Top by default.
    - <a id="ui.widgets.Stack.settings.visible" name="ui.widgets.Stack.settings.visible"></a>**`visible`** (any, optional). Whether the widget is visible, or a signal that holds that value. A widget that is not visible keeps no room and leaves no gap. [`Blank`](#ui.widgets.Blank) keeps the room instead.

  Returns: [trx.ui.Widget](#ui.Widget). The stack.

- <a id="ui.widgets.List" name="ui.widgets.List"></a>[lua]`trx.ui.widgets.List(settings)`  
  A column of rows that the player picks one entry from. Its lengths are in
  canvas units at the default text size.

  The list keeps the cursor and the scroll position. Read the player's input
  with [`trx.ui.List:control`](#ui.List.control) from the input callback of the layer that holds
  the list.

  Parameters:
  - <a id="ui.widgets.List.settings" name="ui.widgets.List.settings"></a>**`settings`** (table). The list settings.

    Keys:
    - <a id="ui.widgets.List.settings.rows" name="ui.widgets.List.settings.rows"></a>**`rows`** (a list of [trx.ui.ListRow](#ui.ListRow), optional). The rows. None by default.
    - <a id="ui.widgets.List.settings.page_rows" name="ui.widgets.List.settings.page_rows"></a>**`page_rows`** (any, optional). How many rows to show at once, or a signal that holds that value. Every row by default.
    - <a id="ui.widgets.List.settings.reserve" name="ui.widgets.List.settings.reserve"></a>**`reserve`** (any, optional). Whether to keep room for the rows of a page when the list holds fewer, or a signal that holds that value. `false` by default.
    - <a id="ui.widgets.List.settings.min_w" name="ui.widgets.List.settings.min_w"></a>**`min_w`** (any, optional). The least width, in canvas units at the default text size, or a signal that holds it.
    - <a id="ui.widgets.List.settings.row_pad" name="ui.widgets.List.settings.row_pad"></a>**`row_pad`** (any, optional). The room on each side of a row's text, or a signal that holds it. `4` by default.
    - <a id="ui.widgets.List.settings.row_spacing" name="ui.widgets.List.settings.row_spacing"></a>**`row_spacing`** (any, optional). The gap between two rows, or a signal that holds it. `3` by default.
    - <a id="ui.widgets.List.settings.row_height" name="ui.widgets.List.settings.row_height"></a>**`row_height`** (any, optional). The height of a row, or a signal that holds it. `15` by default.
    - <a id="ui.widgets.List.settings.hint_height" name="ui.widgets.List.settings.hint_height"></a>**`hint_height`** (any, optional). The height of the arrows that say the list runs past its rows, or a signal that holds it. `7` by default.
    - <a id="ui.widgets.List.settings.rule_overhang" name="ui.widgets.List.settings.rule_overhang"></a>**`rule_overhang`** (any, optional). How far a rule between rows reaches past each side of the list, or a signal that holds it. `7` by default.
    - <a id="ui.widgets.List.settings.frame_overhang" name="ui.widgets.List.settings.frame_overhang"></a>**`frame_overhang`** (any, optional). How far the frame of the row under the cursor reaches above and below the row, or a signal that holds it. `0` by default.
    - <a id="ui.widgets.List.settings.scroll_hints" name="ui.widgets.List.settings.scroll_hints"></a>**`scroll_hints`** (any, optional). Whether to show arrows where the list runs past its rows, or a signal that holds that value. `true` by default.
    - <a id="ui.widgets.List.settings.visible" name="ui.widgets.List.settings.visible"></a>**`visible`** (any, optional). Whether the widget is visible, or a signal that holds that value. A widget that is not visible keeps no room and leaves no gap. [`Blank`](#ui.widgets.Blank) keeps the room instead.

  Returns: [trx.ui.List](#ui.List). The list.

  Example:
  ```lua
  local list = trx.ui.widgets.List({
    rows = { { text = "Yes" }, { text = "No" } },
  })
  ```

- <a id="ui.widgets.Model" name="ui.widgets.Model"></a>[lua]`trx.ui.widgets.Model(settings)`  
  An object's model, fitted into a box and lit as the game lights a model held
  up to the camera.

  Parameters:
  - <a id="ui.widgets.Model.settings" name="ui.widgets.Model.settings"></a>**`settings`** (table). The model settings.

    Keys:
    - <a id="ui.widgets.Model.settings.object" name="ui.widgets.Model.settings.object"></a>**`object`** (any). The object to draw, or a signal carrying it.
    - <a id="ui.widgets.Model.settings.w" name="ui.widgets.Model.settings.w"></a>**`w`** (any). The width of its box, in canvas units at the default text size, or a signal that holds it.
    - <a id="ui.widgets.Model.settings.h" name="ui.widgets.Model.settings.h"></a>**`h`** (any). The height of its box, in canvas units at the default text size, or a signal that holds it.
    - <a id="ui.widgets.Model.settings.size" name="ui.widgets.Model.settings.size"></a>**`size`** (any, optional). How large to draw the model within its box, from `0` to `1`, or a signal or [`trx.ui.Tween`](#ui.Tween) carrying it. The box keeps its room whatever the size. `1` by default.
    - <a id="ui.widgets.Model.settings.rot_x" name="ui.widgets.Model.settings.rot_x"></a>**`rot_x`** (any, optional). How far to tilt the model, or a signal or [`trx.ui.Tween`](#ui.Tween) carrying it.
    - <a id="ui.widgets.Model.settings.rot_y" name="ui.widgets.Model.settings.rot_y"></a>**`rot_y`** (any, optional). How far to turn the model, or a signal or [`trx.ui.Tween`](#ui.Tween) carrying it.
    - <a id="ui.widgets.Model.settings.rot_z" name="ui.widgets.Model.settings.rot_z"></a>**`rot_z`** (any, optional). How far to roll the model, or a signal or [`trx.ui.Tween`](#ui.Tween) carrying it.
    - <a id="ui.widgets.Model.settings.spin" name="ui.widgets.Model.settings.spin"></a>**`spin`** (any, optional). How far the model turns each tick, on top of `rot_y` , or a signal carrying it. It turns smoothly between ticks, and picks up where it stopped when the spin starts again.
    - <a id="ui.widgets.Model.settings.mesh_mask" name="ui.widgets.Model.settings.mesh_mask"></a>**`mesh_mask`** (any, optional). Which of its meshes to draw, one bit each, or a signal that holds the mask. Every mesh by default.
    - <a id="ui.widgets.Model.settings.light" name="ui.widgets.Model.settings.light"></a>**`light`** (any, optional). The light to draw it in, or a signal carrying it, as [`trx.ui.primitive.model`](#ui.primitive.model) takes it.
    - <a id="ui.widgets.Model.settings.visible" name="ui.widgets.Model.settings.visible"></a>**`visible`** (any, optional). Whether the widget is visible, or a signal that holds that value. A widget that is not visible keeps no room and leaves no gap. [`Blank`](#ui.widgets.Blank) keeps the room instead.

  Returns: [trx.ui.Widget](#ui.Widget). The model.

  Example:
  ```lua
  trx.ui.widgets.Model({
    object = trx.catalog.objects.small_medipack_option,
    w = 64,
    h = 48,
    spin = 4 * trx.math.DEG_1,
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
    - <a id="ui.widgets.SleekBar.settings.visible" name="ui.widgets.SleekBar.settings.visible"></a>**`visible`** (any, optional). Whether the widget is visible, or a signal that holds that value. A widget that is not visible keeps no room and leaves no gap. [`Blank`](#ui.widgets.Blank) keeps the room instead.

  Returns: [trx.ui.Widget](#ui.Widget). The bar.

  Example:
  ```lua
  local held = trx.signal.new(0)
  local bar = trx.ui.widgets.SleekBar({ progress = held })
  ```
