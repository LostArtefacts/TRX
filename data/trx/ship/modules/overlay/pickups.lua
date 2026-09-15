-- Shows pickups that slide in at the corner of the screen.
--
-- Announcements use the first free cell, slide in from the right, remain for a
-- short time, and slide out again. Models free their cells as they leave.
--
-- Models move once per tick through mesh slots. Sprites are drawn directly in
-- their cells and do not slide, matching the original behaviour.

local primitive = trx.ui.primitive
local signal = trx.signal

local ROWS = 3
local COLUMNS = 4
local MAX = ROWS * COLUMNS

local EASE_IN_TICKS = trx.game.LOGIC_FPS // 2
local DISPLAY_TICKS = trx.game.LOGIC_FPS * 2
local EASE_OUT_TICKS = trx.game.LOGIC_FPS
local SPIN_PER_TICK = 4 * trx.math.DEG_1
local WHITE = trx.math.color("ffffff")

local shown = {}

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

-- Returns the cell position and progress in canvas units.
local function box_of(entry)
  local canvas = trx.ui.canvas
  local height = canvas.height * trx.config.get("ui.pickup_scale") / 6
  local width = height * 5 / 4
  local margin_y = canvas.height / 16
  local margin_x = margin_y * 4 / 3
  local gap_x = width / 8
  local gap_y = height / 8

  local from_x = canvas.width + margin_x + width
  local from_y = canvas.height - margin_y - height / 2
  local to_x = canvas.width
    - margin_x
    - width / 2
    - (width + gap_x) * entry.column
  local to_y = canvas.height
    - margin_y
    - height / 2
    - (height + gap_y) * entry.row

  local x = from_x + (to_x - from_x) * entry.ease
  local y = from_y + (to_y - from_y) * entry.ease
  return x - width / 2, y - height / 2, width, height
end

-- Returns the model the ring uses, or nil when a sprite must stand in for it.
local function model_of(object)
  if not trx.config.get("visuals.enable_3d_pickups") then
    return nil
  end
  local icon = trx.inventory.icon_of(object)
  if icon == nil or not trx.objects.get(icon).loaded then
    return nil
  end
  if primitive.mesh_bounds(icon) == nil then
    return nil
  end
  return icon
end

-- Returns the ring's starting angle for the object.
local function start_angle(icon)
  local entry = trx.inventory.ring_item(icon)
  return entry ~= nil and entry.y_rot_sel or 0
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

local function forget(entry)
  if entry.slot ~= nil then
    entry.slot:release()
    entry.slot = nil
  end
end

local function add(object)
  if #shown >= MAX then
    return
  end
  local column, row = free_cell()
  if column == nil then
    return
  end

  local icon = model_of(object)
  local slot = icon ~= nil and trx.ui.mesh_slot() or nil
  -- Every slot taken leaves no model to move, so a sprite stands in for it as
  -- it does for an object that carries no model at all.
  if slot == nil then
    icon = nil
  end

  local entry = {
    object = object,
    icon = icon,
    column = column,
    row = row,
    phase = "ease_in",
    elapsed = 0,
    total = 0,
    -- A sprite does not slide, so it starts where it ends up.
    ease = icon == nil and 1 or 0,
    angle = icon ~= nil and start_angle(icon) or 0,
    slot = slot,
  }
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

  if entry.icon == nil then
    entry.ease = 1
  elseif entry.phase == "ease_in" then
    entry.ease = ease(entry.elapsed / EASE_IN_TICKS)
  elseif entry.phase == "display" then
    entry.ease = 1
  else
    entry.ease = 1 - ease(entry.elapsed / EASE_OUT_TICKS)
  end
  return true
end

local function enabled()
  return trx.game.is_playing and trx.config.get("ui.show_pickups_overlay")
end

signal.tick:on(function()
  if #shown == 0 then
    return
  end

  if trx.game.is_playing then
    local kept = {}
    for _, entry in ipairs(shown) do
      if advance(entry) then
        kept[#kept + 1] = entry
      else
        forget(entry)
      end
    end
    shown = kept
  end

  local visible = enabled()
  for _, entry in ipairs(shown) do
    if entry.slot ~= nil then
      if visible then
        local x, y, width, height = box_of(entry)
        entry.slot:move({
          object = entry.icon,
          x = x,
          y = y,
          w = width,
          h = height,
          rot_y = entry.angle + SPIN_PER_TICK * entry.total,
        })
      else
        entry.slot:hide()
      end
    end
  end
end)

-- Paints a sprite where a model would be, scaled to fill the same cell.
trx.events.on_ui_paint(function()
  if not enabled() then
    return
  end
  for _, entry in ipairs(shown) do
    if entry.icon == nil and trx.objects.get(entry.object).loaded then
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
  for _, entry in ipairs(shown) do
    forget(entry)
  end
  shown = {}
end)
