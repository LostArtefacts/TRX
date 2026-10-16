-- What the screens of an inventory ring entry share: the entry the player
-- picked comes to rest before its screen shows or reads any input.
--
-- Usage:
--   local ring = require("common.ui.ring")
--   local resting = ring.resting()
--   ctx:on_end(function() resting:stop() end)
--   ctx:push({ root = ..., on_input = ring.while_resting(control) })

local M = {}

-- What the open ring says over the caption, and where the entry the player
-- picked rests, which the screens of its entries read.
M.signals = {
  -- The parts of the key hint, as the text that shows.
  hint = trx.signal.new({}),
  -- The count that stands with the caption, or nil for none.
  count = trx.signal.new(nil),
  -- The name of the ring on show, at the top of the screen, or nil for none.
  heading = trx.signal.new(nil),
  -- Whether the ring asks for Lara's health bar, as it does while it rests
  -- on a medipack.
  health_bar = trx.signal.new(false),
  -- The frame the picked entry rests open on, or nil while it moves or no
  -- entry is picked.
  rest_frame = trx.signal.new(nil),
}

-- The ring that is open, or nil.
local active = nil

-- The entry a script asked the ring to rest on next, or nil.
---@type integer?
local requested = nil

-- Tells the screens that a ring has opened, or closed.
function M.attach(state)
  active = state
end

function M.detach(state)
  if active == state then
    active = nil
  end
end

-- Asks the ring to rest on an entry once it next builds itself, as after
-- combining two entries into what they make.
function M.request(object)
  requested = object
end

-- Returns the entry asked for, and forgets the request.
function M.take_request()
  local object = requested
  requested = nil
  return object
end

-- Shows key hints where the ring shows its own, each as a role and what it
-- does, up to four.
---@param hints { input: trx.input.Role, text: string }[]
function M.set_hint(hints)
  local parts = {}
  for i, hint in ipairs(hints) do
    parts[i] = trx.ui.input_hint_text(hint.input, hint.text)
  end
  local shown = M.signals.hint:get()
  local same = #shown == #parts
  for i = 1, #parts do
    same = same and shown[i] == parts[i]
  end
  if not same then
    M.signals.hint:set(parts)
  end
end

function M.set_count(text)
  if M.signals.count:get() ~= text then
    M.signals.count:set(text)
  end
end

function M.set_heading(text)
  if M.signals.heading:get() ~= text then
    M.signals.heading:set(text)
  end
end

function M.show_health_bar(shown)
  if M.signals.health_bar:get() ~= shown then
    M.signals.health_bar:set(shown)
  end
end

function M.set_rest_frame(frame)
  if M.signals.rest_frame:get() ~= frame then
    M.signals.rest_frame:set(frame)
  end
end

local function is_resting()
  return trx.inventory_ring.signals.rest_frame:get() ~= nil
end

-- Returns a signal that holds whether the entry rests open.
function M.resting()
  return trx.inventory_ring.signals.rest_frame:map(function(frame)
    return frame ~= nil
  end)
end

-- Returns an input handler that runs `control` only while the entry rests
-- open, and reads no input while the entry moves.
function M.while_resting(control)
  return function(layer, keys)
    if is_resting() then
      control(layer, keys)
    end
  end
end

return M
