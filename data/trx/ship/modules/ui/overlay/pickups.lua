-- Shows pickups that slide in at the corner of the screen.
--
-- Announcements use the first free cell, slide in from the right, remain for a
-- short time, and slide out again. Models free their cells as they leave.
--
-- Models slide and turn smoothly between ticks. Sprites are drawn in their
-- cells and do not slide, matching the original behaviour.

local primitive = trx.ui.primitive
local signal = trx.signal

local ROWS = 3
local COLUMNS = 4
local MAX = ROWS * COLUMNS

local EASE_IN_TICKS = trx.game.LOGIC_FPS // 2
local DISPLAY_TICKS = trx.game.LOGIC_FPS * 2
local EASE_OUT_TICKS = trx.game.LOGIC_FPS
local SPIN_PER_TICK = 4 * trx.math.DEG_1

-- Every mesh of an object, for one the ring states no selection for.
local ALL_MESHES = 0xFFFFFFFF
local WHITE = trx.math.color("ffffff")

local shown = {}

-- How many of an entry's own ticks have gone by at the frame being drawn,
-- part way between its last two. An entry counts only the ticks the game is
-- played in, so it waits out the inventory and the pause.
local function ticks_drawn(count)
  return count - 1 + trx.ui.interpolation_rate()
end

-- How far an entry has slid in, from 0 off the screen to 1 in its cell.
local function slid(entry)
  if entry.icon == nil then
    return 1
  end
  local function eased(ticks)
    local ratio = ticks_drawn(entry.elapsed) / ticks
    return trx.ui.ease.quad_in_out(math.min(math.max(ratio, 0), 1))
  end
  if entry.phase == "ease_in" then
    return eased(EASE_IN_TICKS)
  elseif entry.phase == "display" then
    return 1
  end
  return 1 - eased(EASE_OUT_TICKS)
end

-- Returns the cell position and progress in canvas units.
local function box_of(entry)
  local canvas = trx.ui.canvas
  local working = trx.ui.working_area
  local bottom = working.y + working.height
  local height = canvas.height * trx.config.get("ui.pickup_scale") / 6
  local width = height * 5 / 4
  local margin_y = canvas.height / 16
  local margin_x = margin_y * 4 / 3
  local gap_x = width / 8
  local gap_y = height / 8

  local from_x = canvas.width + margin_x + width
  local from_y = bottom - margin_y - height / 2
  local to_x = canvas.width
    - margin_x
    - width / 2
    - (width + gap_x) * entry.column
  local to_y = bottom - margin_y - height / 2 - (height + gap_y) * entry.row

  local ratio = slid(entry)
  local x = from_x + (to_x - from_x) * ratio
  local y = from_y + (to_y - from_y) * ratio
  return x - width / 2, y - height / 2, width, height
end

local function has_sprite(object)
  return primitive.sprite_count(object) > 0
end

-- Returns the model the ring uses, or nil when the object has none.
local function model_of(object)
  local icon = trx.inventory_ring.icon_of(object)
  if icon == nil or not trx.objects.get(icon).loaded then
    return nil
  end
  if primitive.mesh_bounds(icon) == nil then
    return nil
  end
  return icon
end

-- Return the ring pose: its starting angle, its fixed rotations, and which of
-- its meshes the ring draws.
local function ring_pose(icon)
  local entry = trx.inventory_ring.item(icon)
  if entry == nil then
    return 0, 0, 0, ALL_MESHES
  end
  return entry.y_rot_sel + entry.base_rot_y,
    entry.base_rot_x,
    entry.base_rot_z,
    entry.meshes_drawn
end

-- Returns a free cell. Models release their cells when they start leaving;
-- sprites hold theirs until they disappear.
local function free_cell()
  for index = 0, MAX - 1 do
    local column = index % COLUMNS
    local row = index // COLUMNS
    local taken = false
    for _, entry in ipairs(shown) do
      local leaving = entry.icon ~= nil and entry.phase == "ease_out"
      if entry.column == column and entry.row == row and not leaving then
        taken = true
        break
      end
    end
    if not taken then
      return column, row
    end
  end
  return nil
end

local function add(object)
  if #shown >= MAX then
    return
  end
  local column, row = free_cell()
  if column == nil then
    return
  end

  -- Draws an object without a sprite as its model, even with 3D pickups off.
  local sprite = has_sprite(object)
  local icon = model_of(object)
  if sprite and not trx.config.get("visuals.enable_3d_pickups") then
    icon = nil
  end
  if icon == nil and not sprite then
    return
  end

  local entry = {
    object = object,
    icon = icon,
    column = column,
    row = row,
    phase = "ease_in",
    elapsed = 0,
    total = 0,
    angle = 0,
    rot_x = 0,
    rot_z = 0,
    mesh_mask = ALL_MESHES,
  }
  if icon ~= nil then
    entry.angle, entry.rot_x, entry.rot_z, entry.mesh_mask = ring_pose(icon)
  end
  shown[#shown + 1] = entry
end

-- Advances an announcement and reports whether it remains on screen.
local function advance(entry)
  entry.elapsed = entry.elapsed + 1
  entry.total = entry.total + 1

  if entry.phase == "ease_in" then
    if entry.elapsed >= EASE_IN_TICKS then
      entry.elapsed = 0
      entry.phase = "display"
    end
  elseif entry.phase == "display" then
    if entry.elapsed >= DISPLAY_TICKS then
      entry.elapsed = 0
      entry.phase = "ease_out"
    end
  elseif entry.elapsed >= EASE_OUT_TICKS then
    return false
  end
  return true
end

local function enabled()
  return trx.game.is_playing and trx.config.get("ui.show_pickups_overlay")
end

signal.tick:on(function()
  if #shown == 0 or not trx.game.is_playing then
    return
  end
  local kept = {}
  for _, entry in ipairs(shown) do
    if advance(entry) then
      kept[#kept + 1] = entry
    end
  end
  shown = kept
end)

-- Paints each model in its cell, or a sprite scaled to fill the cell where
-- there is no model.
trx.ui.on_paint(function()
  if not enabled() then
    return
  end
  for _, entry in ipairs(shown) do
    if entry.icon ~= nil then
      local x, y, width, height = box_of(entry)
      local spin = SPIN_PER_TICK * ticks_drawn(entry.total)
      primitive.model(
        entry.icon,
        x,
        y,
        width,
        height,
        entry.rot_x,
        math.floor(entry.angle + spin) % (4 * trx.math.DEG_90),
        entry.rot_z,
        entry.mesh_mask
      )
    else
      local x0, y0, x1, y1 = primitive.sprite_bounds(entry.object, 0)
      local sprite_w = math.abs(x1 - x0)
      local sprite_h = math.abs(y1 - y0)
      if sprite_w > 0 and sprite_h > 0 then
        local x, y, width, height = box_of(entry)
        local scale = math.min(height / sprite_h, width / sprite_w)
        primitive.sprite(
          entry.object,
          0,
          x + (width - sprite_w * scale) / 2 - x0 * scale,
          y + (height - sprite_h * scale) / 2 - y0 * scale,
          0,
          scale,
          WHITE
        )
      end
    end
  end
end)

trx.events.on_show_pickup(add)

-- Clears announcements when the level ends.
trx.events.on_level_unload(function()
  shown = {}
end)
