-- Combining an entry in the inventory ring: what it combines with, in a row,
-- slid along to the one wanted, in a frame that names it.

local ring = require("common.ui.ring")

local M = {}

-- Fallback text for the screen's own strings. The strings files and their
-- translations take precedence.
trx.locale.declare({
  ["general/actions/cancel"] = "Cancel",
})

-- The room the frame keeps for the row.
local ROW_WIDTH = 260.0
local ROW_HEIGHT = 150.0
local FRAME_PAD = 10.0

-- The box the entry in the middle is drawn in, and how far apart the middles
-- of two neighbours stand.
local MODEL_WIDTH = 100.0
local MODEL_HEIGHT = 120.0
local SPACING = 90.0

local SPIN_PER_TICK = 4 * trx.math.DEG_1

-- How far an entry no longer in the middle turns back each tick, as the ring
-- turns back its own entries.
local SPIN_BACK_PER_TICK = 512

-- How far an entry turns about its own axis as the ring poses it.
local function rest_turn(object)
  local entry = ring.item(object)
  return entry and entry.y_rot_sel + entry.base_rot_y or 0
end

-- The model of an entry as the ring poses it, turning while it is in the
-- middle, and turning back to face the player once it leaves the middle.
local function model(object, turn)
  local entry = ring.item(object)
  return trx.ui.widgets.Model({
    object = object,
    w = MODEL_WIDTH,
    h = MODEL_HEIGHT,
    rot_x = entry and entry.base_rot_x or 0,
    rot_y = turn,
    rot_z = entry and entry.base_rot_z or 0,
    mesh_mask = entry and entry.meshes_drawn or nil,
  })
end

local function name_of(object)
  local info = object ~= nil and trx.objects[object] or nil
  return info ~= nil and info.name or ""
end

local function dialog(state)
  local selected = state.selected
  local partners = state.partners
  state.row = trx.ui.widgets.Carousel({
    items = partners,
    selected = selected,
    build = function(object, index)
      return model(object, state.turns[index])
    end,
    w = ROW_WIDTH,
    h = ROW_HEIGHT,
    spacing = SPACING,
  })
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
          state.row,
          trx.ui.widgets.Label({
            text = state.landed:map(function(at)
              return name_of(partners[at])
            end),
          }),
        },
      }),
    }),
  })
end

-- Shows the row once the entry has come to rest, or puts the entry away
-- where it combines with nothing.
local function begin(ctx, state)
  if #state.partners == 0 then
    ctx:cancel()
    return
  end
  state.shown:set(true)
  ring.set_darkened(true)
  -- The frame names the entry in the middle, so the ring names nothing. The
  -- caption goes before the frame first shows, or the frame would move once
  -- the caption gave up its room.
  trx.overlay.set_caption()
  trx.sound.play(trx.catalog.samples.MENU_SPININ)
  ring.set_hint({
    {
      input = trx.input.Role.MENU_CONFIRM,
      text = trx.locale.get("general/actions/combine_item"),
    },
    {
      input = trx.input.Role.MENU_BACK,
      text = trx.locale.get("general/actions/cancel"),
    },
  })
end

local function turn(state, step)
  state.row:turn(step)
  trx.sound.play(trx.catalog.samples.MENU_ROTATE)
end

-- Turns the entry in the middle a step further, and the others a step back.
local function spin(state)
  local wrap = trx.math.wrap_angle
  for i, object in ipairs(state.partners) do
    local turn = state.turns[i]
    local at = turn:target()
    if i == state.landed:get() then
      turn:to(at + SPIN_PER_TICK, 1)
    else
      local off = wrap(at - rest_turn(object))
      local back = math.min(math.abs(off), SPIN_BACK_PER_TICK)
      turn:to(off > 0 and at - back or at + back, 1)
    end
  end
end

local function control(ctx, state, keys)
  if not state.shown:get() then
    begin(ctx, state)
    return
  end
  local sliding = state.row:is_sliding()
  if not sliding then
    state.landed:set(state.selected:get())
  end
  spin(state)
  if sliding then
    return
  end
  local many = #state.partners > 1
  if many and keys:held(trx.input.Role.MENU_RIGHT) then
    turn(state, 1)
  elseif many and keys:held(trx.input.Role.MENU_LEFT) then
    turn(state, -1)
  elseif keys:pressed(trx.input.Role.MENU_CONFIRM) then
    -- The ring rests on what the two made once it settles again.
    ring.request(
      trx.inventory.combine(ctx.object, state.partners[state.selected:get()])
    )
    ctx:confirm()
  elseif keys:pressed(trx.input.Role.MENU_BACK) then
    -- The ring sounds the entry going away as it puts it away.
    ctx:cancel()
  end
end

---@param ctx trx.ui.EntryContext
local function open(ctx)
  local state = {
    partners = trx.inventory.combine_partners(ctx.object),
    selected = trx.signal.new(1),
    -- The entry the row last came to rest on. The frame names it and spins
    -- it until the next entry reaches the middle, where the selection moves
    -- as soon as the row starts to slide.
    landed = trx.signal.new(1),
    turns = {},
    shown = trx.signal.new(false),
  }
  for i, object in ipairs(state.partners) do
    state.turns[i] = trx.ui.tween(rest_turn(object))
  end
  ctx:on_end(function()
    ring.set_darkened(false)
  end)
  -- Nothing shows until the entry comes to rest and the row opens.
  return ctx:push({
    root = dialog(state),
    on_input = ring.while_resting(function(_, keys)
      control(ctx, state, keys)
    end),
  })
end

function M.setup()
  trx.ui.screens.define(trx.ui.Screen.COMBINE, open)
end

return M
