-- A button that the player holds down to act, so that one stray key press
-- cannot do something that cannot be undone, such as deleting a save. It is
-- its words and the key to hold, over a bar that fills while the key is held.
--
-- Usage:
--   local hold_button = require("common.ui.widgets.hold_button")
--   local button = hold_button.new({
--     words = trx.locale.get("general/passport/delete_save"),
--     align = trx.ui.HAlign.RIGHT,
--   })
--   ctx:push({ root = button.widget, ... })
--   if button:control(keys, can_delete) then ... end

local metrics = require("common.ui.metrics")

local M = {}

local ROLE = trx.input.Role.UNBIND_KEY

local TEXT_SCALE = 0.85
local PAD_X = 6.0
local PAD_Y = 3.0
local SPACING = 2.0
local BAR_HEIGHT = 4.0

-- The ticks a hold takes before the bar starts to fill, and then until it is
-- full.
local HOLD_DEBUFF = 10
local HOLD_MAX = 30

-- The height that the button takes, in canvas units at the default text size.
M.HEIGHT = 2.0 * PAD_Y
  + metrics.TEXT_HEIGHT * TEXT_SCALE
  + SPACING
  + BAR_HEIGHT

-- A button: its widget, and the hold it counts.
---@class common.ui.HoldButton
---@field widget trx.ui.Widget The button, to push or to place in another widget.
---@field private has_key boolean
---@field private enabled trx.signal.Signal
---@field private hidden trx.signal.Signal
---@field private fill trx.ui.Tween
---@field private filling trx.signal.Signal
---@field private empty trx.signal.Signal
---@field private hold integer
---@field private armed boolean
local Button = {}
Button.__index = Button

-- The words and the key to hold, or nil while the key is not bound. The
-- player cannot bind keys while a screen with the button is open, so the
-- button reads this once, as it is built.
local function label_of(words)
  local key = trx.input.key_name(ROLE)
  if key == nil then
    return nil
  end
  return string.format(
    "%s: %s",
    words,
    trx.locale.format("general/misc/hold_fmt", key)
  )
end

-- A hold counts one a tick, so the bar fills in one move from the tick it
-- starts to fill, and glides between ticks.
function Button:sync()
  local filling = self.hold >= HOLD_DEBUFF
  if filling and not self.filling:get() then
    self.fill:set(0)
    self.fill:to(1, HOLD_MAX)
  elseif not filling and self.filling:get() then
    self.fill:set(0)
  end
  self.filling:set(filling)
end

-- Shows or hides the button. Hiding it starts the hold again.
function Button:set_enabled(enabled)
  if not enabled then
    self.hold = 0
  end
  self:sync()
  self.enabled:set(enabled and self.has_key)
end

-- Reads one tick of input, and returns whether the hold has just completed.
-- While `enabled` is false the button is hidden and reads no input. After a
-- completed hold, the count waits for the key to be let go.
function Button:control(keys, enabled)
  local fired = false
  if enabled then
    local held = keys:held_for(ROLE)
    if held == 0 then
      self.armed = true
    end
    self.hold = self.armed and held or 0
    fired = self.hold - HOLD_DEBUFF > HOLD_MAX
    if fired then
      self.armed = false
      self.hold = 0
    end
  end
  self:set_enabled(enabled)
  return fired
end

-- Builds the button: its words over the bar, set against one side of the
-- width it is given. It keeps its room while it is disabled or its key is not
-- bound.
local function build(self, text, align, scale)
  local bar =
    trx.ui.widgets.SleekBar({ progress = self.fill, hidden = self.empty })
  return trx.ui.widgets.Stack({
    align = align,
    children = {
      trx.ui.widgets.Pad({
        x = PAD_X * scale,
        y = PAD_Y * scale,
        hidden = self.hidden,
        child = trx.ui.widgets.Stack({
          spacing = SPACING,
          align = trx.ui.HAlign.SPAN,
          children = {
            trx.ui.widgets.Label({
              text = text,
              scale = TEXT_SCALE * scale,
            }),
            bar,
          },
        }),
      }),
    },
  })
end

---@class common.ui.HoldButton.settings
---@field words string What the button does.
---@field align? trx.ui.HAlign Where the button sits in the width it is given. Centered by default.
---@field scale? number How many times its size the button is drawn. `1` by default.
---@field from? common.ui.HoldButton A button that this one stands in for, whose hold it carries on.

-- Creates a button with no hold under way. It starts hidden.
---@param settings common.ui.HoldButton.settings
---@return common.ui.HoldButton
function M.new(settings)
  local from = settings.from
  local enabled = trx.signal.new(false)
  local filling = trx.signal.new(from ~= nil and from.filling:get() or false)
  local label = label_of(settings.words)
  local self = setmetatable({
    has_key = label ~= nil,
    enabled = enabled,
    hidden = ~enabled,
    fill = from ~= nil and from.fill or trx.ui.tween(0),
    filling = filling,
    empty = ~filling,
    hold = from ~= nil and from.hold or 0,
    armed = from == nil or from.armed,
  }, Button)
  self:sync()
  self.widget = build(
    self,
    label or "",
    settings.align or trx.ui.HAlign.CENTER,
    settings.scale or 1.0
  )
  return self
end

return M
