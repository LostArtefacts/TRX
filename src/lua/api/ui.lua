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

api.enum("ui.FrameStyle", {
  backing = "UI_FRAME_STYLE",
  description = "The look a frame draws around what it holds.",
  values = {
    DIALOG_BACKGROUND = "The background a dialog sits on.",
    DIALOG_BACKGROUND_HEAVY = "The same background, drawn heavier.",
    DIALOG_HEADING = "The band a dialog puts its title in.",
    SELECTED_OPTION = "The highlight on the option the player is on.",
    OUTLINE_ONLY = "An outline, with nothing behind it.",
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

api.define("ui.label", {
  description = "Draws a line of text. It takes the player's font and text size, and the text "
    .. "markup applies.",
  params = {
    {
      name = "text",
      type = "string",
      description = "What to draw.",
    },
    {
      name = "settings",
      type = "table",
      optional = true,
      description = "How to draw it.",
      fields = {
        {
          name = "scale",
          type = "number",
          optional = true,
          description = "How much to multiply the text size by. `1.0` by default.",
        },
        {
          name = "z",
          type = "integer",
          optional = true,
          description = "What to draw in front of. Higher draws later. `0` by default.",
        },
      },
    },
  },
  impl = raw.label,
})

api.define("ui.measure", {
  description = "Answers how much room a line of text takes, without drawing it. Unlike the rest "
    .. "of the module, this is available at any time.",
  params = {
    {
      name = "text",
      type = "string",
      description = "What to measure.",
    },
    {
      name = "settings",
      type = "table",
      optional = true,
      description = "The settings `trx.ui.label` would draw it with.",
      fields = {
        {
          name = "scale",
          type = "number",
          optional = true,
          description = "How much to multiply the text size by. `1.0` by default.",
        },
      },
    },
  },
  returns = {
    { type = "number", description = "The width, in canvas units." },
    { type = "number", description = "The height, in canvas units." },
  },
  impl = raw.measure,
})

api.define("ui.spacer", {
  description = "Leaves a gap. It draws nothing and takes the room it is given.",
  params = {
    {
      name = "w",
      type = "number",
      description = "The width, in canvas units.",
    },
    {
      name = "h",
      type = "number",
      description = "The height, in canvas units.",
    },
  },
  impl = raw.spacer,
})

api.define("ui.bar", {
  description = "Draws one of the game's own bars, in the player's chosen bar style.",
  params = {
    {
      name = "settings",
      type = "table",
      description = "Which bar to draw and how full it is.",
      fields = {
        {
          name = "type",
          type = "ui.BarType",
          optional = true,
          description = "Which bar it is. `trx.ui.BarType.LARA_HP` by default.",
        },
        {
          name = "value",
          type = "integer",
          optional = true,
          description = "How much is filled. `0` by default.",
        },
        {
          name = "max_value",
          type = "integer",
          optional = true,
          description = "What a full bar holds. `100` by default.",
        },
        {
          name = "w",
          type = "integer",
          optional = true,
          description = "The width, in canvas units. The game's own width by default.",
        },
        {
          name = "h",
          type = "integer",
          optional = true,
          description = "The height, in canvas units. The game's own height by default.",
        },
      },
    },
  },
  examples = {
    [[trx.ui.bar({
  type = trx.ui.BarType.PROGRESS,
  value = 40,
  max_value = 100,
})]],
  },
  impl = raw.bar,
})

api.define("ui.stack", {
  description = "Lays several widgets out in a row or a column.",
  params = {
    {
      name = "settings",
      type = "table",
      description = "How to lay the children out.",
      fields = {
        {
          name = "orientation",
          type = "ui.Orientation",
          optional = true,
          description = "Which way the children run. `trx.ui.Orientation.VERTICAL` by default.",
        },
        {
          name = "align",
          type = "table",
          optional = true,
          description = "Where the children sit within the stack.",
          fields = {
            {
              name = "h",
              type = "ui.HAlign",
              optional = true,
              description = "Where they sit across its width.",
            },
            {
              name = "v",
              type = "ui.VAlign",
              optional = true,
              description = "Where they sit down its height.",
            },
          },
        },
        {
          name = "spacing",
          type = "table",
          optional = true,
          description = "The gap left between one child and the next.",
          fields = {
            {
              name = "h",
              type = "number",
              optional = true,
              description = "The horizontal gap, in canvas units.",
            },
            {
              name = "v",
              type = "number",
              optional = true,
              description = "The vertical gap, in canvas units.",
            },
          },
        },
      },
    },
    {
      name = "body",
      type = "function",
      description = "Draws the children.",
    },
  },
  examples = {
    [[trx.ui.stack({
  orientation = trx.ui.Orientation.HORIZONTAL,
  spacing = { h = 4 },
}, function()
  trx.ui.label("00:12")
  trx.ui.label("\{small}elapsed")
end)]],
  },
  impl = raw.stack,
})

api.define("ui.anchor", {
  description = "Puts what it holds at a spot in the room it is given. The spot is a ratio, so "
    .. "`0.5, 0.5` is the middle and `1.0, 0.0` is the top right.",
  params = {
    {
      name = "x",
      type = "number",
      description = "How far across, from 0 to 1.",
    },
    {
      name = "y",
      type = "number",
      description = "How far down, from 0 to 1.",
    },
    { name = "body", type = "function", description = "Draws what it holds." },
  },
  impl = raw.anchor,
})

api.define("ui.pad", {
  description = "Keeps a border clear around what it holds.",
  params = {
    {
      name = "settings",
      type = "table",
      description = "How much to keep clear, in canvas units.",
      fields = {
        {
          name = "x",
          type = "number",
          optional = true,
          description = "How much to keep clear at the left and the right. `0` by default.",
        },
        {
          name = "y",
          type = "number",
          optional = true,
          description = "How much to keep clear at the top and the bottom. `0` by default.",
        },
        {
          name = "l",
          type = "number",
          optional = true,
          description = "The left side on its own, which wins over the pair.",
        },
        {
          name = "r",
          type = "number",
          optional = true,
          description = "The right side on its own, which wins over the pair.",
        },
        {
          name = "t",
          type = "number",
          optional = true,
          description = "The top on its own, which wins over the pair.",
        },
        {
          name = "b",
          type = "number",
          optional = true,
          description = "The bottom on its own, which wins over the pair.",
        },
      },
    },
    { name = "body", type = "function", description = "Draws what it holds." },
  },
  impl = raw.pad,
})

api.define("ui.hide", {
  description = "Draws what it holds, or not. What it holds takes its room either way, so a "
    .. "widget that comes and goes does not move the widgets beside it.",
  params = {
    {
      name = "hidden",
      type = "boolean",
      description = "Whether to leave it undrawn.",
    },
    { name = "body", type = "function", description = "Draws what it holds." },
  },
  impl = raw.hide,
})

api.define("ui.resize", {
  description = "Gives what it holds a size of its own.",
  params = {
    {
      name = "settings",
      type = "table",
      description = "The size to give it, in canvas units.",
      fields = {
        {
          name = "w",
          type = "number",
          optional = true,
          description = "The width. Its own width by default; `0` hides it but keeps its room.",
        },
        {
          name = "h",
          type = "number",
          optional = true,
          description = "The height. Its own height by default; `0` hides it but keeps its room.",
        },
        {
          name = "align_h",
          type = "number",
          optional = true,
          description = "Where it sits across the width, from 0 to 1. `0` by default.",
        },
        {
          name = "align_v",
          type = "number",
          optional = true,
          description = "Where it sits down the height, from 0 to 1. `0` by default.",
        },
      },
    },
    { name = "body", type = "function", description = "Draws what it holds." },
  },
  impl = raw.resize,
})

api.define("ui.frame", {
  description = "Draws one of the game's frames behind what it holds.",
  params = {
    {
      name = "style",
      type = "ui.FrameStyle",
      description = "Which frame to draw.",
    },
    { name = "body", type = "function", description = "Draws what it holds." },
  },
  impl = raw.frame,
})

api.define("ui.offset", {
  description = "Moves what it holds. The room it takes does not move with it, so the widgets "
    .. "beside it stay where they are.",
  params = {
    {
      name = "x",
      type = "number",
      description = "How far to move it across, in canvas units.",
    },
    {
      name = "y",
      type = "number",
      description = "How far to move it down, in canvas units.",
    },
    { name = "body", type = "function", description = "Draws what it holds." },
  },
  impl = raw.offset,
})

api.define("ui.span", {
  description = "Draws what it holds one on top of the next, every one as big as the biggest.",
  params = {
    { name = "body", type = "function", description = "Draws what it holds." },
  },
  impl = raw.span,
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
