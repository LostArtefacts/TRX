local raw = trxc.ui
local raw_events = trxc.events
local h = require("trx.internal.helpers")

---@class (partial) trx
---@field ui trx.ui

---Module for drawing on top of the game.
---
---Every function here is available only from a `trx.ui.on_draw` handler, and
---raises anywhere else: the interface is built afresh each drawn frame, and
---there is no scene to add to outside one.
---
---A handler adds to the region the game is building, which it is told the name
---of. Widgets land in the same stack as the health bars and the item names, so
---a script cannot draw over them and the player's choice of where each element
---sits still holds.
---
---Widgets that hold other widgets take the body as a function rather than
---opening and closing by hand, so a scene stays whole even where the body
---fails.
---
---Sizes are in canvas units, not screen pixels. `trx.ui.canvas` reports the
---canvas, and `trx.ui.safe_area` the part of it that is free to draw in.
---
---Text carries the same markup the rest of the game uses, and it is part of
---this API: `\{small}` draws the rest of the line small, `\{arrow up}` draws an
---arrow, and `\{button left}` draws the button the player has bound.
---@trx.module 20 User interface
---@class (partial,exact) trx.ui
---@trx.readonly canvas, safe_area, text_scale, working_area
---@field canvas trx.ui.Area The whole canvas. Widget sizes are in these units rather than in screen pixels, and the canvas is 640 by 480 for a 4:3 screen at the default text size.
---@field clipboard string What the system clipboard holds. Reads as an empty string where it holds nothing, and raises on assignment where the platform refuses the text.
---
---  A script-drawn text field uses this value to paste and copy text.
---@field text_scale number The scale applied to text and its boxes. The value depends on the player's text size and the screen. It is not the `ui.text_scale` setting alone. <!--noref: ui.text_scale-->
---@field safe_area trx.ui.Area The part of the canvas that is free to draw in: the canvas, less the margin kept at the edges, less what the game reserves at the top and the bottom for the bars and the text it puts there.
---@field working_area trx.ui.Area The part of the canvas between the cinematic bars. It is the whole canvas while there are none, and follows the bars as they move in and out. The interface regions lay out inside it, so `trx.ui.safe_area` lies within it too.
local M = h.module("ui")

---The direction a stack lays its children out in.
---@enum trx.ui.Orientation
local Orientation = {
  ---One below the next.
  VERTICAL = h.IntegerConstant,
  ---One beside the next.
  HORIZONTAL = h.IntegerConstant,
}
M.Orientation = h.enum("ui.Orientation", "UI_STACK_ORIENTATION", Orientation)

---Where a stack puts its children across its width.
---@enum trx.ui.HAlign
local HAlign = {
  ---Against the left edge.
  LEFT = h.IntegerConstant,
  ---In the middle.
  CENTER = h.IntegerConstant,
  ---Against the right edge.
  RIGHT = h.IntegerConstant,
  ---Stretched to the full width.
  SPAN = h.IntegerConstant,
  ---Spread out, with the gaps taking the spare width.
  DISTRIBUTE = h.IntegerConstant,
}
M.HAlign = h.enum("ui.HAlign", "UI_STACK_H_ALIGN", HAlign)

---Where a stack puts its children down its height.
---@enum trx.ui.VAlign
local VAlign = {
  ---Against the top edge.
  TOP = h.IntegerConstant,
  ---In the middle.
  CENTER = h.IntegerConstant,
  ---Against the bottom edge.
  BOTTOM = h.IntegerConstant,
  ---Stretched to the full height.
  SPAN = h.IntegerConstant,
  ---Spread out, with the gaps taking the spare height.
  DISTRIBUTE = h.IntegerConstant,
}
M.VAlign = h.enum("ui.VAlign", "UI_STACK_V_ALIGN", VAlign)

---One of the nine places the interface is built in. A handler is told which
---one is being built and adds to it, and everything asking for a place is laid
---out together there rather than over what else asked for it.
---
---The eight around the edge stack what they hold away from the edge they sit
---at. The middle is what the others leave, and is where a dialog goes.
---@enum trx.ui.Region
local Region = {
  ---The top left corner.
  TOP_LEFT = h.IntegerConstant,
  ---The top edge, in the middle.
  TOP_CENTER = h.IntegerConstant,
  ---The top right corner.
  TOP_RIGHT = h.IntegerConstant,
  ---The left edge, halfway down.
  LEFT = h.IntegerConstant,
  ---The middle of the screen, inside what the others leave.
  CENTER = h.IntegerConstant,
  ---The right edge, halfway down.
  RIGHT = h.IntegerConstant,
  ---The bottom left corner.
  BOTTOM_LEFT = h.IntegerConstant,
  ---The bottom edge, in the middle.
  BOTTOM_CENTER = h.IntegerConstant,
  ---The bottom right corner.
  BOTTOM_RIGHT = h.IntegerConstant,
}
M.Region = h.enum("ui.Region", "UI_REGION", Region)

---Whether a widget is drawn below or above the engine interface.
---
---Widgets use the lower layer by default. Use the upper layer for a console or
---a text field. Each region keeps space for both layers.
---@enum trx.ui.Layer
local Layer = {
  ---Below the engine interface.
  UNDER = h.IntegerConstant,
  ---Above the engine interface.
  OVER = h.IntegerConstant,
}
M.Layer = h.enum("ui.Layer", "UI_PAINT_LAYER", Layer)

-- The engine fires an event as it builds each region and as it paints each
-- layer. The types are reflected out of ENUM_MAP, as trx.events reads them.
local types = {}
for _, constant in ipairs(trxc.enum.values("LUA_EVENT_TYPE")) do
  types[constant.name] = constant.value
end
local Listener = h.class_of("events.Listener")

local function listen(event_type)
  return function(callback)
    return setmetatable(
      { _id = raw_events.attach(event_type, callback) },
      Listener
    )
  end
end

---Happens while the interface is built, once for each region. The handler
---adds widgets to the region it is handed, or reserves room in it to paint
---into later.
---
---It happens anywhere the game draws its interface, including fades, FMVs
---and normal play, and it follows the frame rate rather than the game clock.
---
---```lua
---local slot = nil
---
---trx.ui.on_draw(function(region)
---  if region == trx.ui.Region.TOP_CENTER then
---    local w, h = trx.ui.primitive.measure_text("hello")
---    slot = trx.ui.primitive.reserve(region, w, h)
---  end
---end)
---
---trx.ui.on_paint(function()
---  local x, y = trx.ui.primitive.slot_box(slot)
---  if x ~= nil then
---    trx.ui.primitive.text("hello", x, y)
---  end
---end)
---```
---@param callback fun(region: trx.ui.Region) What to run for each region.
---@trx.arg callback.region The region being built.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(region: trx.ui.Region)): trx.events.Listener
M.on_draw = listen(types.UI_DRAW)

---Happens once the interface is laid out and before it is drawn, under the
---engine interface. The boxes reserved during `trx.ui.on_draw` are known by
---then, and the primitive drawing calls work here and nowhere else.
---@param callback function What to run when it happens.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_paint = listen(types.UI_PAINT)

---`trx.ui.on_paint` for the layer above the engine interface. A script paints
---here where its work must cover the interface rather than sit under it, such
---as a console or a text field.
---
---`trx.ui.regions.place` picks the layer for a widget, so a script building
---with widgets has no reason to take this.
---@param callback function What to run when it happens.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_paint_over = listen(types.UI_PAINT_OVER)

---Which of the game's frames to draw. The look of each follows the menu style
---the player chose.
---@enum trx.ui.FrameStyle
local FrameStyle = {
  ---The box a dialog sits in.
  DIALOG = h.IntegerConstant,
  ---The box a dialog sits in, drawn solid.
  DIALOG_HEAVY = h.IntegerConstant,
  ---The strip a dialog puts its title in.
  HEADING = h.IntegerConstant,
  ---The box around the option the player is on.
  SELECTED = h.IntegerConstant,
  ---An outline with nothing behind it.
  OUTLINE = h.IntegerConstant,
}
M.FrameStyle = h.enum("ui.FrameStyle", "UI_FRAME_STYLE", FrameStyle)

---Which of the game's bars to draw, which decides its colors.
---@enum trx.ui.BarType
local BarType = {
  ---Lara's health.
  LARA_HP = h.IntegerConstant,
  ---Lara's health while she is poisoned.
  LARA_HP_POISON = h.IntegerConstant,
  ---Lara's air.
  LARA_AIR = h.IntegerConstant,
  ---Lara's stamina.
  LARA_STAMINA = h.IntegerConstant,
  ---Lara's exposure to the cold.
  LARA_EXPOSURE = h.IntegerConstant,
  ---An enemy's health.
  ENEMY_HP = h.IntegerConstant,
  ---An ally's health.
  ALLY_HP = h.IntegerConstant,
  ---A general progress bar.
  PROGRESS = h.IntegerConstant,
}
M.BarType = h.enum("ui.BarType", "UI_BAR_TYPE", BarType)

---A rectangle on the canvas, in canvas units, counted from the top left.
---@trx.record
---@class trx.ui.Area
---@field x number The left edge.
---@field y number The top edge.
---@field width number How wide it is.
---@field height number How tall it is.

h.properties(M, "ui", {
  canvas = {
    get = function()
      return {
        x = 0,
        y = 0,
        width = raw.get_canvas_width(),
        height = raw.get_canvas_height(),
      }
    end,
  },
  clipboard = {
    get = raw.get_clipboard,
    set = raw.set_clipboard,
  },
  text_scale = {
    get = raw.text_scale,
  },
  safe_area = {
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
  },
  working_area = {
    get = function()
      local top = raw.get_working_top()
      return {
        x = 0,
        y = top,
        width = raw.get_canvas_width(),
        height = raw.get_working_bottom() - top,
      }
    end,
  },
})

---The part of the canvas that a box of the given width, centered, is free to
---take. Unlike `trx.ui.safe_area`, an edge of the interface takes room from it
---only if the box is wide enough to reach that edge, so a narrow box ignores
---what sits in a corner, such as the FPS counter.
---
---```lua
---local area = trx.ui.safe_area_for(200)
---```
---@param width number How wide the box is, in canvas units.
---@return trx.ui.Area # The room the box has.
function M.safe_area_for(width)
  local safe_width = raw.get_safe_width()
  local top = raw.get_safe_top(width)
  return {
    x = (raw.get_canvas_width() - safe_width) / 2,
    y = top,
    width = safe_width,
    height = raw.get_safe_bottom(width) - top,
  }
end

---A model the interface keeps on screen across ticks. Move it once per tick;
---the engine blends between its current and previous poses when it draws each
---frame. The fields report the current tick's pose.
---@class (exact) trx.ui.MeshSlot
---@trx.readonly h, object, rot_x, rot_y, rot_z, visible, w, x, y
---@field object trx.catalog.objects The object drawn in the slot.
---@field visible boolean Whether the model is drawn.
---@field x number The left edge of the box, in canvas units.
---@field y number The top edge of the box, in canvas units.
---@field w number How wide the box is, in canvas units.
---@field h number How tall the box is, in canvas units.
---@field rot_x trx.math.Angle How far the model is tilted.
---@field rot_y trx.math.Angle How far the model is turned.
---@field rot_z trx.math.Angle How far the model is rolled.
local MeshSlot = h.handle("ui.MeshSlot", "UI_MESH_SLOT", {
  fields = {
    object = "object_id",
    visible = "visible",
    x = "x",
    y = "y",
    w = "w",
    h = "h",
    rot_x = "rot_x",
    rot_y = "rot_y",
    rot_z = "rot_z",
  },
})

---Puts the model where it should be at the end of this tick, and shows it.
---
---Takes a table of `object` <!--noref: object-->, `x` <!--noref: x-->,
---`y` <!--noref: y-->, `w` <!--noref: w-->, `h` <!--noref: h-->,
---`rot_x` <!--noref: rot_x-->, `rot_y` <!--noref: rot_y--> and
---`rot_z` <!--noref: rot_z-->. The box uses canvas units. An omitted value is
---zero. Each turn takes the short way around the angle wrap.
---
---Call this once per tick. Calling it twice in one tick replaces the pose used
---for interpolation. A hidden slot, or one given a new object, starts at the
---new pose.
function MeshSlot:move()
  return h.native()
end

---Stops drawing the model. Moving the slot again shows it.
function MeshSlot:hide()
  return h.native()
end

---Gives the slot back. The handle is spent afterwards, and moving or hiding a
---spent handle raises rather than reaching whichever slot came next. Releasing
---one again does nothing.
function MeshSlot:release()
  return h.native()
end

---Takes a slot for a model the interface keeps on screen across ticks.
---
---Take a slot once, when a script loads, and give it back with
---`trx.ui.MeshSlot:release` when nothing needs it. Returns nothing where every
---slot is taken.
---@return trx.ui.MeshSlot # The slot, or `nil` where none is free.
---@type fun(): trx.ui.MeshSlot
M.mesh_slot = raw.mesh_slot
