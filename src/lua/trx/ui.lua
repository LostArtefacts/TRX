local raw = trxc.ui
local raw_events = trxc.events
local h = require("trx.internal.helpers")
local themes = require("trx.internal.themes")

---@class (partial) trx
---@field ui trx.ui

---Module for drawing on top of the game.
---
---A script builds its interface from widgets once, and keeps them.
---`trx.ui.regions.place` puts a widget in one of the regions of the screen,
---where it stacks with the health bars and the item names, so the player's
---choice of where each element sits still holds. `trx.ui.layers.push` puts a
---screen of widgets, such as a menu, over the rest. A widget takes signals in
---place of fixed values, and follows them as they change.
---
---Below the widgets, a script can reserve room in a region during
---`trx.ui.on_reserve` and paint into it by hand with `trx.ui.primitive` during
---`trx.ui.on_paint`. The primitive drawing calls work only while the
---interface is painted.
---
---Sizes are in canvas units, not screen pixels. `trx.ui.canvas` reports the
---canvas, and `trx.ui.safe_area` the part of it that is free to draw in.
---
---Text carries the same markup the rest of the game uses, and it is part of
---this API: `\{small}` draws the rest of the line small, `\{arrow up}` draws an
---arrow, and `\{button left}` draws the button the player has bound.
---`\{color #c08040 #401000}` draws what follows in a colour that grades from
---the top of each glyph to its bottom, up to `\{/color}`. A colour is
---`#rrggbb` or `#rrggbbaa`, one for a flat colour or two for a gradient, and
---`pulse` after them makes it brighten and dim. In TR4, a channel of `80`
---draws a glyph as it is, and higher values light it up, as the original
---lights its font. <!--noref: pulse-->
---@trx.module 20 User interface
---@class (partial,exact) trx.ui
---@trx.readonly canvas, safe_area, screen, text_scale, working_area
---@field canvas trx.ui.Area The whole canvas. Widget sizes are in these units rather than in screen pixels, and the canvas is 640 by 480 for a 4:3 screen at the default text size.
---@field clipboard string What the system clipboard holds. Reads as an empty string where it holds nothing, and raises on assignment where the platform refuses the text.
---
---  A script-drawn text field uses this value to paste and copy text.
---@field text_scale number The scale applied to text and its boxes. The value depends on the player's text size and the screen. It is not the `ui.text_scale` setting alone. <!--noref: ui.text_scale-->
---@field screen trx.ui.Area The whole screen, in canvas units. The canvas holds a whole number of units, so its right and bottom edges can stop short of the screen by a fraction of a unit; the screen reaches the edges. Something drawn from edge to edge, such as a frame around the screen, uses this area.
---@field safe_area trx.ui.Area The part of the canvas that is free to draw in: the canvas, less the margin kept at the edges, less what the game reserves at the top and the bottom for the bars and the text it puts there.
---@field frames_shown boolean Whether the game's frames are drawn: the boxes of dialogs, the strips of headings and the boxes around the option the player is on. A script that draws its menus flat turns them off, and the option the player is on then shows in `trx.ui.picked_text_color` instead. `true` by default.
---@field text_color string? The colour text is drawn in where it names none, as the text markup names a colour, or `nil` for the font as it is drawn.
---@field picked_text_color string? The colour the option the player is on shows in while `trx.ui.frames_shown` is off, as the text markup names a colour, or `nil` for the colour of the text around it.
---@field working_area trx.ui.Area The part of the canvas between the cinematic bars. It is the whole canvas while there are none, and follows the bars as they move in and out. The interface regions lay out inside it, so `trx.ui.safe_area` lies within it too.
local M = h.module("ui")

---The direction a stack lays its children out in.
---@enum trx.ui.Orientation
local Orientation = {
  ---One below the next.
  VERTICAL = 0,
  ---One beside the next.
  HORIZONTAL = 1,
}
M.Orientation = h.coined_enum("ui.Orientation", Orientation)

---Where a stack puts its children across its width.
---@enum trx.ui.HAlign
local HAlign = {
  ---Against the left edge.
  LEFT = 0,
  ---In the middle.
  CENTER = 1,
  ---Against the right edge.
  RIGHT = 2,
  ---Stretched to the full width.
  SPAN = 3,
  ---Spread out, with the gaps taking the spare width.
  DISTRIBUTE = 4,
}
M.HAlign = h.coined_enum("ui.HAlign", HAlign)

---Where a stack puts its children down its height.
---@enum trx.ui.VAlign
local VAlign = {
  ---Against the top edge.
  TOP = 0,
  ---In the middle.
  CENTER = 1,
  ---Against the bottom edge.
  BOTTOM = 2,
  ---Stretched to the full height.
  SPAN = 3,
  ---Spread out, with the gaps taking the spare height.
  DISTRIBUTE = 4,
}
M.VAlign = h.coined_enum("ui.VAlign", VAlign)

---One of the nine areas of the screen that the interface is laid out in.
---Everything that reserves room in a region is stacked together, so nothing in
---one region overlaps.
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

-- The engine fires an event as it builds each region and as it paints. The
-- types are reflected out of ENUM_MAP, as trx.events reads them.
local types = h.constants("LUA_EVENT_TYPE")
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
---reserves room in the region it is handed with `trx.ui.primitive.reserve`,
---and paints into that room later during `trx.ui.on_paint`.
---
---It happens anywhere the game draws its interface, including fades, FMVs
---and normal play, and it follows the frame rate rather than the game clock.
---
---```lua
---local slot = nil
---
---trx.ui.on_reserve(function(region)
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
M.on_reserve = listen(types.UI_RESERVE)

---Happens once the interface is laid out and before it is drawn, under the
---engine interface. The boxes reserved during `trx.ui.on_reserve` are known by
---then, and the primitive drawing calls work here and nowhere else. The
---console still draws over what is painted here.
---@param callback function What to run when it happens.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_paint = listen(types.UI_PAINT)

---Which of the game's frames to draw. The look of each follows the menu style
---the player chose.
---@enum trx.ui.FrameStyle
local FrameStyle = {
  ---The box a dialog sits in.
  DIALOG = 0,
  ---The box a dialog sits in, drawn solid.
  DIALOG_HEAVY = 1,
  ---The strip a dialog puts its title in.
  HEADING = 2,
  ---The box around the option the player is on.
  SELECTED = 3,
  ---An outline with nothing behind it.
  OUTLINE = 4,
  ---The outline of the strip a dialog puts its title in, with nothing behind
  ---it, as the game draws around a color swatch.
  HEADING_OUTLINE = 5,
}
M.FrameStyle = h.coined_enum("ui.FrameStyle", FrameStyle)

---Which of the game's bars to draw, which decides its colors.
---@enum trx.ui.BarType
local BarType = {
  ---Lara's health.
  LARA_HP = 0,
  ---Lara's health while she is poisoned.
  LARA_HP_POISON = 1,
  ---Lara's air.
  LARA_AIR = 2,
  ---Lara's stamina.
  LARA_STAMINA = 3,
  ---Lara's exposure to the cold.
  LARA_EXPOSURE = 4,
  ---An enemy's health.
  ENEMY_HP = 5,
  ---An ally's health.
  ALLY_HP = 6,
  ---A general progress bar.
  PROGRESS = 7,
}
M.BarType = h.coined_enum("ui.BarType", BarType)

---A rectangle on the canvas, in canvas units, counted from the top left.
---@trx.record
---@class trx.ui.Area
---@field x number The left edge.
---@field y number The top edge.
---@field width number How wide it is.
---@field height number How tall it is.

local frames_shown = true
local text_color = nil
local picked_text_color = nil

h.properties(M, "ui", {
  text_color = {
    get = function()
      return text_color
    end,
    set = function(color)
      raw.set_text_color(color)
      text_color = color
    end,
  },
  picked_text_color = {
    get = function()
      return picked_text_color
    end,
    set = function(color)
      picked_text_color = color
    end,
  },
  frames_shown = {
    get = function()
      return frames_shown
    end,
    set = function(shown)
      frames_shown = shown == true
    end,
  },
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
    get = raw.drawn_text_scale,
  },
  screen = {
    get = function()
      return {
        x = 0,
        y = 0,
        width = raw.get_screen_width(),
        height = raw.get_screen_height(),
      }
    end,
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

---What the interface tells a script about the room that it lays out in.
---@class (exact) trx.ui.signals
---@trx.readonly layout
---@field layout trx.signal.Signal Says when the room that the interface lays out in changes: the canvas, `trx.ui.safe_area`, `trx.ui.working_area` or `trx.ui.text_scale`. It holds a count that goes up with each change. A widget that lays itself out by that room listens to this rather than reading the room every frame.
M.signals = h.namespace("ui.signals")

-- The engine says when the room changes, so the signal follows without a
-- poll. The handler is attached as the module loads, so it belongs to no
-- level: one that a level script attached would go with the level.
local layout_count = 0
---@type trx.signal.Signal?
local layout = nil
local function on_layout()
  layout_count = layout_count + 1
  if layout ~= nil then
    layout:set(layout_count)
  end
end

local function layout_signal()
  if layout == nil then
    layout = trx.signal.new(layout_count)
  end
  return layout
end

h.properties(M.signals, "ui.signals", {
  layout = { get = layout_signal },
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

---Whether a bar of the given type takes the PS1 look, which the player picks
---in the settings and which decides which of its colors applies.
---@param type trx.ui.BarType The bar.
---@return boolean # True for the PS1 look.
function M.is_ps1_bar(type)
  return themes.is_ps1(type)
end

---Returns the interpolation factor of the frame being drawn between the last
---two ticks: `0` at the earlier tick and `1` at the later one.
---`trx.ui.Tween` reads it to move smoothly between ticks.
---@return number # The factor, from `0` to `1`.
---@type fun(): number
M.interpolation_rate = raw.interpolation_rate

-- The name the text markup knows each input role by, built on first use so
-- that trx.ui loads without trx.input.
local role_names = nil

local function role_name(role)
  if role_names == nil then
    role_names = {}
    for name, value in pairs(trx.input.Role) do
      role_names[value] = name:lower()
    end
  end
  local name = role_names[role]
  if name == nil then
    error(("unknown input role: %s"):format(tostring(role)), 3)
  end
  return name
end

---One input role, or a list of roles shown side by side.
---@alias trx.ui.Inputs trx.input.Role|trx.input.Role[]

---The text that shows the keys bound to one or more input roles, as the game
---shows them, followed by some words. The keys follow the player's bindings
---and controller.
---
---```lua
---local text = trx.ui.input_hint_text(trx.input.Role.MENU_CONFIRM, "Use")
---```
---@param input trx.ui.Inputs The role, or the roles shown side by side.
---@overload fun(input: trx.ui.Inputs, words: trx.locale.Text): trx.locale.Text
---@overload fun(input: trx.ui.Inputs, words?: string|trx.locale.Text): string|trx.locale.Text
---@param words? string What the keys do. A `trx.locale.Text` makes the result follow the player's language.
---@return string # The text.
function M.input_hint_text(input, words)
  local roles = type(input) == "table" and input or { input }
  local parts = {}
  for i, role in ipairs(roles) do
    parts[i] = "\\{input " .. role_name(role) .. "}"
  end
  local keys = table.concat(parts)
  if words == nil or words == "" then
    return keys
  end
  return keys .. " " .. words
end

raw_events.attach(types.UI_LAYOUT, on_layout)
