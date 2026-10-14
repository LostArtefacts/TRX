---
title: Building a user interface
order: 2
---

# Building a user interface

Use the smallest part of the UI API that fits the job:

1. `trx.ui.widgets` builds the interface.
2. `trx.ui.layers` puts a temporary or interactive screen above it.
3. `trx.ui.regions` puts persistent HUD content beside the engine's interface.
4. `trx.ui.screens` replaces one of the engine's screens.
5. `Custom` and `primitive` are escape hatches for work the widgets do not cover.

Build widgets once. Give changing settings signals. Place a root widget in a
region or give it to a layer. The region or layer owns it, and releases it when
it goes away. A widget owns its children, so ordinary scripts do not release
children separately.

## Sizes

Sizes are in canvas units. Most widget lengths are given at the default text
size, and follow the player's text size: the margins of `Pad`, the room of
`Spacer`, the gaps and rows of `Stack` and `List`, and the boxes of `Model` and
`Carousel`. A `Bar` follows the player's bar size instead. `Image` and `Resize`
take their sizes as they are, and so does `Custom`, which measures itself. A
layer's placement is on the canvas, so its margin does not follow the text
size.

## A persistent HUD widget

```lua
local label = trx.ui.widgets.Label({ text = "Hello" })
trx.ui.regions.place(trx.ui.Region.TOP_LEFT, label)
```

Use a region when the widget should share space with the engine's HUD. A
level's placement ends with that level. A script that places session-long UI
must own its lifetime explicitly.

## A temporary dialog

```lua
local layer = trx.ui.layers.push({
  root = trx.ui.widgets.Frame({
    child = trx.ui.widgets.Pad({
      x = 12,
      y = 8,
      child = trx.ui.widgets.Label({ text = "Press cancel" }),
    }),
  }),
  on_input = function(layer, keys)
    if keys:pressed(trx.input.Role.MENU_BACK) then
      layer:close()
    end
  end,
})
```

An input layer takes the devices from the game while it is reading input, so
the game does not act on the same menu key. The level keeps running. The layer
closes when the level ends, or when its code calls `close`.

For a selectable list, use `trx.ui.widgets.List:control(keys)` from the layer's
`on_input` callback. The executable menu example shows the complete pattern:
[ui/menu.lua](examples/ui/menu.lua).

## A changing value

```lua
local visible = trx.lara.signals.is_poisoned
local label = trx.ui.widgets.Label({
  text = "Poisoned",
  shown = visible,
})
trx.ui.regions.place(trx.ui.Region.TOP_RIGHT, label)
```

A widget follows every signal it reads while it measures. A derived signal
lives while something holds it, such as a widget that reads it, and one that
a level script made ends with the level. A `Signal:on` listener stays attached
until it is detached; use that for session-long engine-signal listeners.

## A custom widget

```lua
local mark = trx.ui.widgets.Custom({
  measure = function()
    return 8, 8
  end,
  paint = function(x, y, w, h)
    trx.ui.primitive.quad(x, y, 0, w, h, trx.math.color("#ffffff"))
  end,
})
trx.ui.regions.place(trx.ui.Region.CENTER, mark)
```

The widget remeasures when a signal that `measure` read changes. Use
primitives only while the UI is being painted; for normal UI, prefer the
widgets above.

## Replacing an engine screen

Use `trx.ui.screens.define` when the engine should hand a whole screen to Lua.
The executable pause-screen example is the complete pattern:
[ui/screen.lua](examples/ui/screen.lua).

## Tested examples

Every example in [the UI cookbook](examples/README.md) is executed by the Lua
documentation tests. The examples are the source of truth for the common
workflows; the generated [UI reference](reference/UI.md) describes every
available setting and method.
