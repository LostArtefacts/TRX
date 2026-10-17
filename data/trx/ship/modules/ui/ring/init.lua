-- What the screens of an inventory ring entry share: the entry the player
-- picked comes to rest before its screen shows or reads any input.
--
-- Usage:
--   local ring = require("common.ui.ring")
--   local resting = ring.resting()
--   ctx:on_end(function() resting:stop() end)
--   ctx:push({ root = ..., on_input = ring.while_resting(control) })

local entries = require("common.ui.ring.entries")

local M = {}

-- Declares how the ring draws an entry, as `common.ui.ring.entries` does.
M.declare_item = entries.declare
M.item = entries.item

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
  -- Whether the ring goes dark behind the screen of the picked entry, as it
  -- does while the frame that combines it shows.
  darkened = trx.signal.new(false),
}

-- The ring that is open, or nil.
---@type table?
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

-- Returns what the open ring was opened for, a `trx.ui.MenuMode`, or nil
-- while no ring is open.
function M.mode()
  return active ~= nil and active.mode or nil
end

-- Returns the object of the entry the open ring rests on, or nil while no
-- ring is open.
function M.selection()
  if active == nil then
    return nil
  end
  local content = active.list[active.view.current]
  return content ~= nil and content.object or nil
end

-- Where the entry the player picked is in its animation, or nil while no
-- ring is open: the frame on show, the frame it heads for, the frame it rests
-- open on, how many frames it holds, and which way it runs.
function M.selection_anim()
  if active == nil then
    return nil
  end
  local index = active.view.current
  local anim = active.anims[index]
  local spec = active.list[index].spec
  return {
    frame = anim.frame,
    goal_frame = anim.goal,
    open_frame = spec.open_frame,
    frame_count = spec.frames_total,
    direction = anim.direction,
  }
end

-- Runs the picked entry's animation to a frame, forwards for a positive
-- direction and backwards otherwise.
function M.animate_selection(goal_frame, direction)
  if active == nil then
    return
  end
  local index = active.view.current
  if goal_frame < 0 or goal_frame >= active.list[index].spec.frames_total then
    error("the entry has no such frame", 2)
  end
  local anim = active.anims[index]
  anim.goal = goal_frame
  anim.direction = direction < 0 and -1 or 1
  if anim.frame ~= anim.goal then
    M.set_rest_frame(nil)
  end
end

-- Turns the entry being examined where no ring is open, or nil.
---@type fun(yaw: number, pitch: number)?
local turner = nil

-- Lets a menu other than the ring, such as the flat menu, turn the entry
-- being examined by hand. Pass nil once the entry is put away.
---@param turn fun(yaw: number, pitch: number)?
function M.set_turner(turn)
  turner = turn
end

-- Turns the picked entry by hand, as the player examines it.
function M.turn_selection(yaw, pitch)
  if active ~= nil then
    active.view:turn_by_hand(yaw, pitch)
  elseif turner ~= nil then
    turner(yaw, pitch)
  end
end

-- Shows key hints where the ring shows its own, each as a role and what it
-- does, up to four.
---@param hints { input: trx.input.Role, text: string }[]
function M.set_hint(hints)
  hints = hints or {}
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

-- Shows how many there are of the entry the ring rests on, where there is
-- more than one.
function M.show_count(count)
  if count > 1 then
    M.set_count(
      trx.locale
        .get("general/inventory_ring/item_count_fmt")
        :format("%d")
        :format(count)
    )
  else
    M.set_count(nil)
  end
end

function M.set_darkened(darkened)
  if M.signals.darkened:get() ~= darkened then
    M.signals.darkened:set(darkened)
  end
end

function M.set_rest_frame(frame)
  if M.signals.rest_frame:get() ~= frame then
    M.signals.rest_frame:set(frame)
  end
end

local function is_resting()
  return M.signals.rest_frame:get() ~= nil
end

-- Returns a signal that holds whether the entry rests open.
function M.resting()
  return M.signals.rest_frame:map(function(frame)
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
