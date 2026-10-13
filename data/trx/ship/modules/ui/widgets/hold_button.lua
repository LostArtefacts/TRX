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
--   ctx:push({ root = button.root, ... })
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

---@class common.ui.HoldButton
---@field root trx.ui.Widget
---@field private words string
---@field private text trx.signal.Signal
---@field private enabled trx.signal.Signal
---@field private hidden trx.signal.Signal
---@field private progress trx.signal.Signal
---@field private empty trx.signal.Signal
---@field private hold integer
---@field private armed boolean
local Button = {}
Button.__index = Button

-- The words and the key to hold, or nil while the key is not bound.
function Button:label()
  local key = trx.input.key_name(ROLE)
  if key == nil then
    return nil
  end
  return string.format(
    "%s: %s",
    self.words,
    trx.locale.format("general/misc/hold_fmt", key)
  )
end

function Button:sync()
  local label = self:label()
  self.text:set(label or "")
  self.progress:set((self.hold - HOLD_DEBUFF) / HOLD_MAX)
  return label
end

-- Shows or hides the button. Hiding it starts the hold again.
function Button:set_enabled(enabled)
  if not enabled then
    self.hold = 0
  end
  local label = self:sync()
  self.enabled:set(enabled and label ~= nil)
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
local function build(self, align, scale)
  local bar = trx.ui.widgets.SleekBar({ progress = self.progress })
  bar.hidden = self.empty
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
              text = self.text,
              scale = TEXT_SCALE * scale,
            }),
            bar,
          },
        }),
      }),
    },
  })
end

-- Creates a button with no hold under way. It starts hidden. The settings
-- are:
--   words - what the button does
--   align - where the button sits in the width it is given, as a
--           trx.ui.HAlign, centered by default
--   scale - how many times its size the button is drawn, 1 by default
function M.new(settings)
  local enabled = trx.signal.new(false)
  local progress = trx.signal.new(-1)
  local self = setmetatable({
    words = settings.words,
    text = trx.signal.new(""),
    enabled = enabled,
    hidden = ~enabled,
    progress = progress,
    empty = progress:map(function(value)
      return value < 0
    end),
    hold = 0,
    armed = true,
  }, Button)
  self.root =
    build(self, settings.align or trx.ui.HAlign.CENTER, settings.scale or 1.0)
  self:sync()
  return self
end

return M
