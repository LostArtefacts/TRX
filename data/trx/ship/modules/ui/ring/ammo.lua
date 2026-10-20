-- Choosing the ammunition of a weapon in the inventory ring: the kinds the
-- weapon takes, side by side in a frame, with the shots of the one picked
-- and the keys that choose it under them. The ring opens the row for a weapon that takes more than
-- one kind, as it offers to examine an entry.

local ammo = require("common.ui.ammo")
local ring = require("common.ui.ring")
local scroll = require("common.ui.scroll")

local M = {}

trx.locale.declare({
  ["general/actions/choose_ammo"] = "Choose ammo",
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

-- The count of the kind picked, which keeps the room of the widest count, so
-- that the frame keeps its size while the player picks a kind.
local function count_label(state)
  local primitive = trx.ui.primitive
  ---@type string[]
  local captions = {}
  for i, kind in ipairs(state.kinds) do
    captions[i] = ammo.caption(state.weapon, kind.kind)
  end
  return trx.ui.widgets.Custom({
    measure = function()
      local width, height = 0.0, 0.0
      for _, caption in ipairs(captions) do
        local w, h = primitive.measure_text(caption, 1.0)
        width = math.max(width, w)
        height = math.max(height, h)
      end
      return width, height
    end,
    paint = function(x, y, w)
      local caption = captions[state.selected:get()] or ""
      local text_w = primitive.measure_text(caption, 1.0)
      primitive.text(caption, x + (w - text_w) / 2, y, 1.0, 0)
    end,
  })
end

local function hints()
  local Role = trx.input.Role
  return trx.ui.widgets.Stack({
    orientation = trx.ui.Orientation.HORIZONTAL,
    spacing = HINT_SPACING,
    children = {
      trx.ui.widgets.InputHint({
        input = Role.MENU_CONFIRM,
        text = trx.locale.get("general/actions/choose_ammo"),
      }),
      trx.ui.widgets.InputHint({
        input = Role.MENU_BACK,
        text = trx.locale.get("general/actions/cancel"),
      }),
    },
  })
end

local function dialog(state)
  local models = {}
  for i, kind in ipairs(state.kinds) do
    models[i] = model(kind.object, i, state.selected, state.turns[i])
  end
  return trx.ui.widgets.Frame({
    style = trx.ui.FrameStyle.OUTLINE,
    visible = state.shown,
    child = trx.ui.widgets.Pad({
      x = FRAME_PAD,
      y = FRAME_PAD,
      child = trx.ui.widgets.Stack({
        spacing = FRAME_PAD,
        align = trx.ui.HAlign.CENTER,
        children = {
          trx.ui.widgets.Stack({
            orientation = trx.ui.Orientation.HORIZONTAL,
            spacing = MODEL_SPACING,
            v_align = trx.ui.VAlign.CENTER,
            children = models,
          }),
          count_label(state),
          hints(),
        },
      }),
    }),
  })
end

-- Shows the row once the weapon has come to rest.
local function begin(state)
  state.shown:set(true)
  ring.set_darkened(true)
  -- The frame names the kind in the middle, so the ring names nothing. The
  -- caption goes before the frame first shows, or the frame would move once
  -- the caption gave up its room.
  trx.overlay.set_caption()
  trx.sound.play(trx.catalog.samples.MENU_SPININ)
  -- The frame shows the count and the hints of its own, so the ring shows
  -- neither while the frame is open.
  ring.set_hint({})
  ring.set_count(nil)
end

-- Picks the kind beside the one picked, as the wraparound setting allows.
local function turn(state, step)
  local at = scroll.step(state.selected:get(), step, #state.kinds)
  if at == nil then
    return
  end
  state.selected:set(at)
  trx.sound.play(trx.catalog.samples.MENU_ROTATE)
end

-- Closes the row, and tells the ring whether a kind was chosen.
local function finish(state, chosen)
  if state.layer == nil then
    return
  end
  local layer = state.layer
  state.layer = nil
  layer:close()
  ring.set_darkened(false)
  state.on_end(chosen)
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
  if keys:pressed(trx.input.Role.MENU_RIGHT) then
    turn(state, 1)
  elseif keys:pressed(trx.input.Role.MENU_LEFT) then
    turn(state, -1)
  elseif keys:pressed(trx.input.Role.MENU_CONFIRM) then
    ammo.choose(state.weapon, state.kinds[state.selected:get()].kind)
    finish(state, true)
  elseif keys:pressed(trx.input.Role.MENU_BACK) then
    -- The ring sounds the entry going away as it puts it away.
    finish(state, false)
  end
end

-- Returns the weapon an inventory entry draws, where the player can choose
-- its ammunition, or nil for any other entry.
---@param object trx.catalog.objects
---@return trx.weapons.Weapon?
function M.weapon_of(object)
  for _, weapon in ipairs(trx.weapons.all) do
    if
      weapon.object ~= nil
      and trx.inventory.icon_of(weapon.object) == object
      and ammo.is_choosable(weapon)
    then
      return weapon
    end
  end
  return nil
end

---@class common.ui.ring.ammo.Choice
---@field cancel fun(self: common.ui.ring.ammo.Choice) Closes the row with no kind chosen.

-- Opens the row of a weapon's ammunition over the ring. Nothing shows until
-- the weapon comes to rest. The row calls `on_end` once it closes, with
-- whether the player chose a kind.
---@param ctx trx.ui.ScreenContext The ring's context, which the row pushes its layer onto.
---@param weapon trx.weapons.Weapon
---@param on_end fun(chosen: boolean)
---@return common.ui.ring.ammo.Choice
function M.open(ctx, weapon, on_end)
  local state = {
    weapon = weapon,
    kinds = ammo.kinds(weapon),
    selected = trx.signal.new(ammo.fired_index(weapon)),
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
