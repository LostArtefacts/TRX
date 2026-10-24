-- What the inventory ring offers on a weapon besides drawing it: choosing its
-- ammunition, and separating the attachment it carries, such as the TR4
-- laser sight. The ring offers these on the key that examines an entry.
--
-- A weapon with one such action and nothing to choose does it at once. A
-- weapon that takes more than one kind of ammunition opens a frame with the
-- kinds side by side and the shots of the one picked under them, and where
-- it offers more actions than that, the actions under the shots.

local accent = require("common.ui.accent")
local ammo = require("common.ui.ammo")
local ring = require("common.ui.ring")
local scroll = require("common.ui.scroll")

local M = {}

trx.locale.declare({
  ["general/actions/choose_ammo"] = "Choose ammo",
  ["general/actions/separate_item"] = "Separate",
  ["general/actions/weapon_options"] = "Options",
  ["general/actions/ammo"] = "Ammo",
  ["general/actions/action"] = "Action",
})

local FRAME_PAD = 10.0

-- The box each kind is drawn in, how much smaller the kinds beside the one
-- picked are drawn, and the gap between two kinds.
local MODEL_WIDTH = 100.0
local MODEL_HEIGHT = 120.0
local SIDE_SIZE = 0.75
local MODEL_SPACING = 0.0

-- The gap between the parts of the row of hints.
local HINT_SPACING = 20.0

-- The colours of the action picked and of the others.

local SPIN_PER_TICK = 4 * trx.math.DEG_1

-- How far a kind no longer picked turns back each tick, as the ring turns
-- back its own entries.
local SPIN_BACK_PER_TICK = 512

-- The light the kind picked is drawn in, as bright as the entry the ring
-- rests on, and the darker light of the kinds beside it.
local PICKED_BRIGHT = 160 / 128 * 255
local PICKED_LIGHT =
  { ambient = { r = PICKED_BRIGHT, g = PICKED_BRIGHT, b = PICKED_BRIGHT } }
local SIDE_LIGHT = { ambient = trx.math.color(96, 96, 96) }

---@alias common.ui.ring.weapon.Action "choose_ammo"|"separate"

local ACTION_TEXT = {
  choose_ammo = "general/actions/choose_ammo",
  separate = "general/actions/separate_item",
}

---@class common.ui.ring.weapon.Choice
---@field weapon trx.weapons.Weapon
---@field object trx.catalog.objects The weapon's inventory entry.
---@field actions common.ui.ring.weapon.Action[] What the player can do with it, in the order the frame lists them.
---@field has_row boolean Whether the player chooses among kinds of ammunition.

-- Returns what the ring offers on an inventory entry besides drawing it, or
-- nil for an entry that is no weapon or offers nothing more.
---@param object trx.catalog.objects
---@return common.ui.ring.weapon.Choice?
function M.choice_of(object)
  for _, weapon in ipairs(trx.weapons.all) do
    if
      weapon.object ~= nil
      and trx.inventory.icon_of(weapon.object) == object
    then
      local has_row = ammo.is_choosable(weapon)
      local actions = {}
      if has_row then
        actions[#actions + 1] = "choose_ammo"
      end
      local offers = trx.inventory.actions(object)
      if offers ~= nil and offers.separate then
        actions[#actions + 1] = "separate"
      end
      if #actions == 0 then
        return nil
      end
      return {
        weapon = weapon,
        object = object,
        actions = actions,
        has_row = has_row,
      }
    end
  end
  return nil
end

-- Returns what the hint of the ring says the key does: the one action where
-- there is one, and the options otherwise.
---@param choice common.ui.ring.weapon.Choice
---@return string
function M.hint_of(choice)
  if #choice.actions == 1 then
    return trx.locale.get(ACTION_TEXT[choice.actions[1]])
  end
  return trx.locale.get("general/actions/weapon_options")
end

-- Returns whether the key does the one action at once, with no frame.
---@param choice common.ui.ring.weapon.Choice
---@return boolean
function M.is_immediate(choice)
  return not choice.has_row and #choice.actions == 1
end

-- Does an action of a weapon, with the kind of ammunition picked on the row.
-- Returns whether the ring holds other entries after it, and asks the ring
-- to rest on the weapon then.
local function perform(choice, action, kind)
  if action == "choose_ammo" and kind ~= nil then
    ammo.choose(choice.weapon, kind)
  elseif action == "separate" then
    if trx.inventory.separate(choice.object) ~= nil then
      ring.request(choice.object)
      return true
    end
  end
  return false
end

-- Does the one action of a weapon at once. Returns whether the ring holds
-- other entries after it.
---@param choice common.ui.ring.weapon.Choice
---@return boolean
function M.perform_at_once(choice)
  return perform(choice, choice.actions[1], nil)
end

-- How far a box turns about its own axis as the ring poses it.
local function rest_turn(object)
  local entry = ring.item(object)
  return entry and entry.y_rot_sel + entry.base_rot_y or 0
end

-- The model of a box as the ring poses it. The kind picked turns, and the
-- others turn back to face the player, smaller and darker.
local function model(object, index, selected, turn)
  local entry = ring.item(object)
  local function picked(value, other)
    return selected:map(function(at)
      return at == index and value or other
    end)
  end
  return trx.ui.widgets.Model({
    object = object,
    w = MODEL_WIDTH,
    h = MODEL_HEIGHT,
    size = picked(1, SIDE_SIZE),
    rot_x = entry and entry.base_rot_x or 0,
    rot_y = turn,
    rot_z = entry and entry.base_rot_z or 0,
    mesh_mask = entry and entry.meshes_drawn or nil,
    light = picked(PICKED_LIGHT, SIDE_LIGHT),
  })
end

-- A line of text that keeps the room of the widest of its texts, so that the
-- frame keeps its size while the player picks.
local function steady_label(texts, picked)
  local primitive = trx.ui.primitive
  return trx.ui.widgets.Custom({
    measure = function()
      local width, height = 0.0, 0.0
      for _, text in ipairs(texts) do
        local w, h = primitive.measure_text(text, 1.0)
        width = math.max(width, w)
        height = math.max(height, h)
      end
      return width, height
    end,
    paint = function(x, y, w)
      local text = texts[picked()] or ""
      local text_w = primitive.measure_text(text, 1.0)
      primitive.text(text, x + (w - text_w) / 2, y, 1.0, 0)
    end,
  })
end

local function count_label(state)
  ---@type string[]
  local captions = {}
  for i, kind in ipairs(state.kinds) do
    captions[i] = accent.text(
      ammo.caption(state.choice.weapon, kind.kind),
      accent.Role.NAME
    )
  end
  return steady_label(captions, function()
    return state.selected:get()
  end)
end

-- The actions, one to a line, the one picked brighter than the others.
local function action_lines(state)
  local lines = {}
  for i, action in ipairs(state.choice.actions) do
    local text = trx.locale.get(ACTION_TEXT[action])
    lines[i] = trx.ui.widgets.Label({
      text = state.action:map(function(at)
        local role = at == i and accent.Role.PICKED or accent.Role.MENU_OPTION
        return accent.text(text, role)
      end),
    })
  end
  return trx.ui.widgets.Stack({
    align = trx.ui.HAlign.CENTER,
    children = lines,
  })
end

local function hint_row(children)
  return trx.ui.widgets.Stack({
    orientation = trx.ui.Orientation.HORIZONTAL,
    spacing = HINT_SPACING,
    children = children,
  })
end

-- The keys the frame answers to, in two rows: the keys that pick, and the
-- keys that end the frame. A part that does not apply is left out, and the
-- confirm key names the action picked.
local function hints(state)
  local Role = trx.input.Role
  local picks = {}
  if state.choice.has_row then
    picks[#picks + 1] = trx.ui.widgets.InputHint({
      input = { Role.MENU_LEFT, Role.MENU_RIGHT },
      text = trx.locale.get("general/actions/ammo"),
    })
  end
  if #state.choice.actions > 1 then
    picks[#picks + 1] = trx.ui.widgets.InputHint({
      input = { Role.MENU_UP, Role.MENU_DOWN },
      text = trx.locale.get("general/actions/action"),
    })
  end
  local ends = {
    trx.ui.widgets.InputHint({
      input = Role.MENU_CONFIRM,
      text = state.action:map(function(at)
        return trx.locale.get(ACTION_TEXT[state.choice.actions[at]])
      end),
    }),
    trx.ui.widgets.InputHint({
      input = Role.MENU_BACK,
      text = trx.locale.get("general/actions/cancel"),
    }),
  }
  local rows = {}
  if #picks > 0 then
    rows[#rows + 1] = hint_row(picks)
  end
  rows[#rows + 1] = hint_row(ends)
  return trx.ui.widgets.Stack({
    align = trx.ui.HAlign.CENTER,
    children = rows,
  })
end

local function dialog(state)
  local children = {}
  if state.choice.has_row then
    local models = {}
    for i, kind in ipairs(state.kinds) do
      models[i] = model(kind.object, i, state.selected, state.turns[i])
    end
    children[#children + 1] = trx.ui.widgets.Stack({
      orientation = trx.ui.Orientation.HORIZONTAL,
      spacing = MODEL_SPACING,
      v_align = trx.ui.VAlign.CENTER,
      children = models,
    })
    children[#children + 1] = count_label(state)
  end
  if #state.choice.actions > 1 then
    children[#children + 1] = action_lines(state)
  end
  children[#children + 1] = hints(state)
  return trx.ui.widgets.Frame({
    style = trx.ui.FrameStyle.DIALOG_HEAVY,
    visible = state.shown,
    child = trx.ui.widgets.Pad({
      x = FRAME_PAD,
      y = FRAME_PAD,
      child = trx.ui.widgets.Stack({
        spacing = FRAME_PAD,
        align = trx.ui.HAlign.CENTER,
        children = children,
      }),
    }),
  })
end

-- Shows the frame once the weapon has come to rest.
local function begin(state)
  state.shown:set(true)
  ring.set_darkened(true)
  -- The frame shows its own text, so the ring shows no caption, count or
  -- hints while the frame is open. The caption goes before the frame first
  -- shows, or the frame would move once the caption gave up its room.
  trx.overlay.set_caption()
  trx.sound.play(trx.catalog.samples.MENU_SPININ)
  ring.set_hint({})
  ring.set_count(nil)
end

-- Picks the entry beside the one picked, as the wraparound setting allows.
local function step(signal, by, count)
  local at = scroll.step(signal:get(), by, count)
  if at == nil then
    return
  end
  signal:set(at)
  trx.sound.play(trx.catalog.samples.MENU_ROTATE)
end

-- Closes the frame, and tells the ring whether it holds other entries now.
local function finish(state, rebuilds)
  if state.layer == nil then
    return
  end
  local layer = state.layer
  state.layer = nil
  layer:close()
  ring.set_darkened(false)
  state.on_end(rebuilds)
end

-- Turns the kind picked a step further, and the others a step back.
local function spin(state)
  local wrap = trx.math.wrap_angle
  for i, kind in ipairs(state.kinds) do
    local turn = state.turns[i]
    local at = turn:target()
    if i == state.selected:get() then
      turn:to(at + SPIN_PER_TICK, 1)
    else
      local off = wrap(at - rest_turn(kind.object))
      local back = math.min(math.abs(off), SPIN_BACK_PER_TICK)
      turn:to(off > 0 and at - back or at + back, 1)
    end
  end
end

local function control(state, keys)
  if not state.shown:get() then
    begin(state)
    return
  end
  spin(state)
  local Role = trx.input.Role
  local choice = state.choice
  if choice.has_row and keys:pressed(Role.MENU_RIGHT) then
    step(state.selected, 1, #state.kinds)
  elseif choice.has_row and keys:pressed(Role.MENU_LEFT) then
    step(state.selected, -1, #state.kinds)
  elseif keys:pressed(Role.MENU_DOWN) then
    step(state.action, 1, #choice.actions)
  elseif keys:pressed(Role.MENU_UP) then
    step(state.action, -1, #choice.actions)
  elseif keys:pressed(Role.MENU_CONFIRM) then
    local kind = choice.has_row and state.kinds[state.selected:get()].kind
      or nil
    finish(state, perform(choice, choice.actions[state.action:get()], kind))
  elseif keys:pressed(Role.MENU_BACK) then
    -- The ring sounds the entry going away as it puts it away.
    finish(state, false)
  end
end

---@class common.ui.ring.weapon.Frame
---@field cancel fun(self: common.ui.ring.weapon.Frame) Closes the frame with nothing done.

-- Opens the frame of a weapon over the ring. Nothing shows until the weapon
-- comes to rest. The frame calls `on_end` once it closes, with whether the
-- ring holds other entries now, as after separating the laser sight.
---@param ctx trx.ui.ScreenContext The ring's context, which the frame pushes its layer onto.
---@param choice common.ui.ring.weapon.Choice
---@param on_end fun(rebuilds: boolean)
---@return common.ui.ring.weapon.Frame
function M.open(ctx, choice, on_end)
  local state = {
    choice = choice,
    kinds = choice.has_row and ammo.kinds(choice.weapon) or {},
    selected = trx.signal.new(
      choice.has_row and ammo.fired_index(choice.weapon) or 1
    ),
    action = trx.signal.new(1),
    turns = {},
    shown = trx.signal.new(false),
    on_end = on_end,
  }
  for i, kind in ipairs(state.kinds) do
    state.turns[i] = trx.ui.tween(rest_turn(kind.object))
  end
  state.layer = ctx:push({
    root = dialog(state),
    on_input = ring.while_resting(function(_, keys)
      control(state, keys)
    end),
  })
  return {
    cancel = function()
      finish(state, false)
    end,
  }
end

return M
