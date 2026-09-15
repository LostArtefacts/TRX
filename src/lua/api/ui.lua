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

api.enum("ui.Layer", {
  backing = "UI_PAINT_LAYER",
  description = [[
Whether a widget is drawn below or above the engine interface.

Widgets use the lower layer by default. Use the upper layer for a console or a
text field. Each region keeps space for both layers.]],
  values = {
    UNDER = "Below the engine interface.",
    OVER = "Above the engine interface.",
  },
})

api.enum("ui.FrameStyle", {
  backing = "UI_FRAME_STYLE",
  description = "Which of the game's frames to draw. The look of each follows the menu style "
    .. "the player chose.",
  values = {
    DIALOG = "The box a dialog sits in.",
    DIALOG_HEAVY = "The box a dialog sits in, drawn solid.",
    HEADING = "The strip a dialog puts its title in.",
    SELECTED = "The box around the option the player is on.",
    OUTLINE = "An outline with nothing behind it.",
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

api.property("ui.clipboard", {
  type = "string",
  description = [[
What the system clipboard holds. Reads as an empty string where it holds
nothing, and raises on assignment where the platform refuses the text.

A script-drawn text field uses this value to paste and copy text.]],
  get = raw.get_clipboard,
  set = raw.set_clipboard,
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

api.type("ui.MeshSlot", {
  backing = "UI_MESH_SLOT",
  description = [[
A model the interface keeps on screen across ticks. Move it once per tick; the
engine blends between its current and previous poses when it draws each frame.
The fields report the current tick's pose.]],

  fields = {
    object = {
      from = "object_id",
      type = "catalog.objects",
      writable = false,
      description = "The object drawn in the slot.",
    },
    visible = {
      from = "visible",
      type = "boolean",
      writable = false,
      description = "Whether the model is drawn.",
    },
    x = {
      type = "number",
      writable = false,
      description = "The left edge of the box, in canvas units.",
    },
    y = {
      type = "number",
      writable = false,
      description = "The top edge of the box, in canvas units.",
    },
    w = {
      type = "number",
      writable = false,
      description = "How wide the box is, in canvas units.",
    },
    h = {
      type = "number",
      writable = false,
      description = "How tall the box is, in canvas units.",
    },
    rot_y = {
      type = "math.Angle",
      writable = false,
      description = "How far the model is turned.",
    },
  },

  methods = {
    move = {
      description = [[
Puts the model where it should be at the end of this tick, and shows it.

Takes a table of `object` <!--noref: object-->, `x` <!--noref: x-->,
`y` <!--noref: y-->, `w` <!--noref: w-->, `h` <!--noref: h--> and
`rot_y` <!--noref: rot_y-->. The box is in canvas units and an omitted value
counts as zero. The turn takes the short way around,
so a model crossing the wrap does not spin back through every angle between.

Call this once per tick. Calling it twice in one tick replaces the pose used for
interpolation. A hidden slot, or one given a new object, starts at the new pose.]],
    },

    hide = {
      description = "Stops drawing the model. Moving the slot again shows it.",
    },

    release = {
      description = [[
Gives the slot back. The handle is spent afterwards, and moving or hiding a
spent handle raises rather than reaching whichever slot came next. Releasing
one again does nothing.]],
    },
  },
})

api.define("ui.mesh_slot", {
  description = [[
Takes a slot for a model the interface keeps on screen across ticks.

Take a slot once, when a script loads, and give it back with
`trx.ui.MeshSlot:release` when nothing needs it. Returns nothing where every
slot is taken.]],
  returns = {
    type = "ui.MeshSlot",
    description = "The slot, or `nil` where none is free.",
  },
  impl = raw.mesh_slot,
})
