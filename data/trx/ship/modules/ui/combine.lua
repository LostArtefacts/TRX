-- Combining an entry in the inventory ring: what it combines with, in a row,
-- slid along to the one wanted, in a frame that names it.
--
-- Usage:
--   require("common.ui.combine").setup()

local M = {}

local L = trx.locale.get

-- Fallback text for the screen's own strings. The strings files and their
-- translations take precedence.
trx.locale.declare({
  ["general/actions/cancel"] = "Cancel",
})

-- The room the frame keeps for the row.
local FRAME_WIDTH = 260.0
local FRAME_HEIGHT = 150.0
local FRAME_PAD = 10.0

-- The cell the entry in the middle takes, the share of it the ones beside it
-- take, and how far apart their middles stand.
local CELL_WIDTH = 100.0
local CELL_HEIGHT = 120.0
local SIDE_SHARE = 0.6
local SPACING = 90.0
-- How far from the middle an entry slides before it is gone.
local REACH = 1.5

local SLIDE_TICKS = trx.game.LOGIC_FPS // 4
local SPIN_PER_TICK = 4 * trx.math.DEG_1
-- Every mesh of an object, for one the ring states no selection for.
local ALL_MESHES = 0xFFFFFFFF

-- Slow at both ends, fastest in the middle.
local function ease(ratio)
  if ratio <= 0 then
    return 0
  elseif ratio >= 1 then
    return 1
  elseif ratio < 0.5 then
    return 2 * ratio * ratio
  end
  local rest = ratio - 1
  return 1 - 2 * rest * rest
end

-- The pose the ring gives an entry: its starting angle, its fixed rotations,
-- and which of its meshes it draws.
local function ring_pose(object)
  local entry = trx.inventory_ring.item(object)
  if entry == nil then
    return 0, 0, 0, ALL_MESHES
  end
  return entry.y_rot_sel + entry.base_rot_y,
    entry.base_rot_x,
    entry.base_rot_z,
    entry.meshes_drawn
end

-- How far an entry stands from the middle of the row, in places, going the
-- short way round so that the row wraps.
local function distance(index, position, count)
  local d = (index - position) % count
  if d > count / 2 then
    d = d - count
  end
  return d
end

-- The share of a full cell an entry takes at a distance from the middle.
local function share(d)
  d = math.abs(d)
  if d <= 1 then
    return 1 + (SIDE_SHARE - 1) * d
  elseif d < REACH then
    return SIDE_SHARE * (REACH - d) / (REACH - 1)
  end
  return 0
end

-- The room the row turns in. It remembers where it was drawn, which is where
-- the row is placed on the next tick.
local function row_box(state, w, h)
  return trx.ui.widgets.Custom({
    measure = function()
      local scale = trx.ui.text_scale
      return w * scale, h * scale
    end,
    paint = function(x, y, box_w, box_h)
      state.box = { x = x, y = y, w = box_w, h = box_h }
    end,
  })
end

-- The name of the entry in the middle.
local function front_name(state)
  local function text()
    local entry = state.entries and state.entries[state.front]
    local object = entry ~= nil and trx.objects[entry.object] or nil
    return object ~= nil and object.name or ""
  end
  return trx.ui.widgets.Custom({
    measure = function()
      return trx.ui.primitive.measure_text(text(), 1.0)
    end,
    paint = function(x, y)
      trx.ui.primitive.text(text(), x, y, 1.0, 0)
    end,
  })
end

local function frame(state)
  state.name = front_name(state)
  return trx.ui.widgets.Frame({
    style = trx.ui.FrameStyle.OUTLINE,
    shown = state.shown,
    child = trx.ui.widgets.Pad({
      x = FRAME_PAD,
      y = FRAME_PAD,
      child = trx.ui.widgets.Stack({
        spacing = FRAME_PAD,
        align = trx.ui.HAlign.CENTER,
        children = {
          row_box(state, FRAME_WIDTH, FRAME_HEIGHT),
          state.name,
        },
      }),
    }),
  })
end

-- Whether the entry has come to rest, which is when the row opens.
local function is_open()
  local anim = trx.inventory_ring.selection_anim()
  return anim ~= nil and anim.frame == anim.goal_frame
end

local function release(state)
  for _, entry in ipairs(state.entries or {}) do
    if entry.slot ~= nil then
      entry.slot:release()
      entry.slot = nil
    end
  end
end

-- Takes a slot for each partner. Returns how many there are.
local function open_row(ctx, state)
  state.entries = {}
  for _, object in ipairs(trx.inventory.combine_partners(ctx.object)) do
    local angle, rot_x, rot_z, mesh_mask = ring_pose(object)
    state.entries[#state.entries + 1] = {
      object = object,
      slot = trx.ui.mesh_slot(),
      angle = angle,
      spin = angle,
      rot_x = rot_x,
      rot_z = rot_z,
      mesh_mask = mesh_mask,
    }
  end
  state.front = 1
  state.position = 1
  state.from = 1
  state.target = 1
  state.elapsed = SLIDE_TICKS
  return #state.entries
end

local function is_sliding(state)
  return state.elapsed < SLIDE_TICKS
end

local function slide(state, step)
  state.from = state.position
  state.target = state.target + step
  state.elapsed = 0
end

-- Moves the row on by a tick and puts every entry where it now stands.
local function place_row(state)
  if is_sliding(state) then
    state.elapsed = state.elapsed + 1
    local ratio = ease(state.elapsed / SLIDE_TICKS)
    state.position = state.from + (state.target - state.from) * ratio
    if not is_sliding(state) then
      state.position = state.target
      state.front = (state.target - 1) % #state.entries + 1
    end
  end

  local box = state.box
  local count = #state.entries
  local scale = trx.ui.text_scale
  for index, entry in ipairs(state.entries) do
    if entry.slot ~= nil then
      local d = distance(index, state.position, count)
      local cell = share(d)
      if index == state.front and not is_sliding(state) then
        entry.spin = (entry.spin + SPIN_PER_TICK) % (4 * trx.math.DEG_90)
      end
      if box == nil or cell <= 0 or (count == 1 and index ~= state.front) then
        entry.slot:hide()
      else
        local w = CELL_WIDTH * scale * cell
        local h = CELL_HEIGHT * scale * cell
        local x = box.x + box.w / 2 + d * SPACING * scale
        local y = box.y + box.h / 2
        entry.slot:move({
          object = entry.object,
          x = x - w / 2,
          y = y - h / 2,
          w = w,
          h = h,
          mesh_mask = entry.mesh_mask,
          rot_x = entry.rot_x,
          rot_y = entry.spin,
          rot_z = entry.rot_z,
        })
      end
    end
  end
end

local function control(ctx, state, keys)
  if state.entries == nil then
    if not is_open() then
      return
    end
    if open_row(ctx, state) == 0 then
      ctx:cancel()
      return
    end
    trx.sound.play(trx.catalog.samples.MENU_SPININ)
    trx.inventory_ring.set_hint(
      "\\{input menu_confirm} " .. L("general/actions/combine_item"),
      "\\{input menu_back} " .. L("general/actions/cancel")
    )
    state.shown:set(true)
  end
  -- The frame names the entry in the middle, so the ring names nothing.
  trx.overlay.set_caption()
  place_row(state)
  state.name:wake()

  if is_sliding(state) then
    return
  end
  local Role = trx.input.Role
  local count = #state.entries
  if keys:held(Role.MENU_RIGHT) and count > 1 then
    slide(state, 1)
    trx.sound.play(trx.catalog.samples.MENU_ROTATE)
  elseif keys:held(Role.MENU_LEFT) and count > 1 then
    slide(state, -1)
    trx.sound.play(trx.catalog.samples.MENU_ROTATE)
  elseif keys:pressed(Role.MENU_CONFIRM) then
    ctx:confirm(state.entries[state.front].object)
  elseif keys:pressed(Role.MENU_BACK) then
    trx.sound.play(trx.catalog.samples.MENU_SPINOUT)
    ctx:cancel()
  end
end

local function open(ctx)
  local state = { shown = trx.signal.new(false) }
  state.root = frame(state)
  return ctx:push({
    root = state.root,
    on_input = function(_, keys)
      control(ctx, state, keys)
    end,
    on_close = function()
      release(state)
    end,
  })
end

function M.setup()
  trx.ui.screens.define(trx.ui.Screen.COMBINE, open)
end

return M
