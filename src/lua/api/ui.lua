local raw = trxc.ui
local api = trx.api

api.module("ui", {
  order = 19,
  title = "User interface",
  description = [[
Module for drawing on top of the game.

Every function here is available only from a `trx.events.on_ui_draw` handler,
and raises anywhere else: the interface is built afresh each drawn frame, and
there is no scene to add to outside one.

A handler adds to the region the game is building, which it is told the name
of. Widgets land in the same stack as the health bars and the item names, so a
script cannot draw over them and the player's choice of where each element sits
still holds.

Widgets that hold other widgets take the body as a function rather than opening
and closing by hand, so a scene stays whole even where the body fails.

Sizes are in canvas units, not screen pixels. `trx.ui.canvas` reports the
canvas, and `trx.ui.safe_area` the part of it that is free to draw in.

Text carries the same markup the rest of the game uses, and it is part of this
API: `\{small}` draws the rest of the line small, `\{arrow up}` draws an arrow,
and `\{button left}` draws the button the player has bound.
]],
})

api.enum("ui.Orientation", {
  backing = "UI_STACK_ORIENTATION",
  description = "The direction a stack lays its children out in.",
  values = {
    VERTICAL = "One below the next.",
    HORIZONTAL = "One beside the next.",
  },
})

api.enum("ui.HAlign", {
  backing = "UI_STACK_H_ALIGN",
  description = "Where a stack puts its children across its width.",
  values = {
    LEFT = "Against the left edge.",
    CENTER = "In the middle.",
    RIGHT = "Against the right edge.",
    SPAN = "Stretched to the full width.",
    DISTRIBUTE = "Spread out, with the gaps taking the spare width.",
  },
})

api.enum("ui.VAlign", {
  backing = "UI_STACK_V_ALIGN",
  description = "Where a stack puts its children down its height.",
  values = {
    TOP = "Against the top edge.",
    CENTER = "In the middle.",
    BOTTOM = "Against the bottom edge.",
    SPAN = "Stretched to the full height.",
    DISTRIBUTE = "Spread out, with the gaps taking the spare height.",
  },
})

api.enum("ui.Region", {
  backing = "UI_REGION",
  description = "One of the nine places the interface is built in. A handler is told which one "
    .. "is being built and adds to it, and everything asking for a place is laid out together "
    .. "there rather than over what else asked for it.\n\nThe eight around the edge stack what "
    .. "they hold away from the edge they sit at. The middle is what the others leave, and is "
    .. "where a dialog goes.",
  values = {
    TOP_LEFT = "The top left corner.",
    TOP_CENTER = "The top edge, in the middle.",
    TOP_RIGHT = "The top right corner.",
    LEFT = "The left edge, halfway down.",
    CENTER = "The middle of the screen, inside what the others leave.",
    RIGHT = "The right edge, halfway down.",
    BOTTOM_LEFT = "The bottom left corner.",
    BOTTOM_CENTER = "The bottom edge, in the middle.",
    BOTTOM_RIGHT = "The bottom right corner.",
  },
})

api.enum("ui.BarType", {
  backing = "UI_BAR_TYPE",
  description = "Which of the game's bars to draw, which decides its colors.",
  values = {
    LARA_HP = "Lara's health.",
    LARA_HP_POISON = "Lara's health while she is poisoned.",
    LARA_AIR = "Lara's air.",
    LARA_STAMINA = "Lara's stamina.",
    LARA_EXPOSURE = "Lara's exposure to the cold.",
    ENEMY_HP = "An enemy's health.",
    ALLY_HP = "An ally's health.",
    PROGRESS = "A general progress bar.",
  },
})

api.type("ui.Area", {
  record = true,
  description = "A rectangle on the canvas, in canvas units, counted from the top left.",
  fields = {
    x = { type = "number", description = "The left edge." },
    y = { type = "number", description = "The top edge." },
    width = { type = "number", description = "How wide it is." },
    height = { type = "number", description = "How tall it is." },
  },
})

api.property("ui.canvas", {
  type = "ui.Area",
  description = "The whole canvas. Widget sizes are in these units rather than in screen "
    .. "pixels, and the canvas is 640 by 480 for a 4:3 screen at the default text size.",
  get = function()
    return {
      x = 0,
      y = 0,
      width = raw.get_canvas_width(),
      height = raw.get_canvas_height(),
    }
  end,
})

api.property("ui.safe_area", {
  type = "ui.Area",
  description = "The part of the canvas that is free to draw in: the canvas, less the margin "
    .. "kept at the edges, less what the game reserves at the top and the bottom for the bars "
    .. "and the text it puts there.",
  get = function()
    local width = raw.get_safe_width()
    local top = raw.get_safe_top()
    return {
      x = (raw.get_canvas_width() - width) / 2,
      y = top,
      width = width,
      height = raw.get_safe_bottom() - top,
    }
  end,
})
