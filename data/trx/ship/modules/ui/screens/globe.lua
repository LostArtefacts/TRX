-- The globe that TR3 picks its next adventure on, over the picture of where
-- the player has just been. It is drawn as the inventory ring drew it: the
-- ring opens with the globe on it and brings it forward, the globe turns to
-- each area the player can go to next, and the ring closes again once one is
-- picked.

local fade = require("common.ui.fade")
local motion = require("common.ui.ring.motion")
local timer = require("common.timer")

local M = {}

-- Fallback text for the screen's own strings. The strings files and their
-- translations take precedence.
trx.locale.declare({
  ["general/globe_select/area_1"] = "Area 1",
  ["general/globe_select/area_2"] = "Area 2",
  ["general/globe_select/area_3"] = "Area 3",
  ["general/globe_select/area_4"] = "Area 4",
  ["general/globe_select/area_5"] = "Area 5",
  ["general/globe_select/area_6"] = "Area 6",
  ["general/inventory_ring/heading_adventure"] = "Adventure",
})

local LOGIC_FPS = trx.game.LOGIC_FPS

-- The screen's fades, in ticks: it comes in from black, the ring fades out to
-- black and rests there a tick, and the screen stays black a while longer
-- before the adventure starts.
local FADE = {
  open_ticks = LOGIC_FPS,
  close_ticks = 8,
  close_hold_ticks = 1,
  wait_ticks = 18,
}
local DEG_90 = trx.math.DEG_90

-------------------------------------------------------------------------------
-- The ring
-------------------------------------------------------------------------------

-- The globe is the ring's one entry, and comes this far forward once picked.
local GLOBE = { y_trans_sel = -176, z_trans_sel = 256 }

local Status = {
  OPENING = "opening",
  OPEN = "open",
  SELECTING = "selecting",
  SELECTED = "selected",
  CLOSING_ITEM = "closing_item",
  EXITING = "exiting",
  CLOSING = "closing",
  FADING_OUT = "fading_out",
  DONE = "done",
}

-- What the ring does once each motion ends.
local AFTER = {
  [Status.OPENING] = Status.OPEN,
  [Status.SELECTING] = Status.SELECTED,
  [Status.EXITING] = Status.EXITING,
  [Status.CLOSING] = Status.FADING_OUT,
}

local function open_ring()
  local ring = {
    view = motion.new({ entries = { GLOBE } }),
    status = Status.OPENING,
  }
  ring.view:start(motion.Motion.OPEN)
  return ring
end

-- Moves the ring on to a status, starting the motion that takes it there.
local function set_status(ring, status, name)
  ring.status = status
  if name ~= nil then
    ring.view:start(name)
  end
end

-------------------------------------------------------------------------------
-- The areas
-------------------------------------------------------------------------------

local function is_finished(area)
  local level = area.completion_level
  return level ~= nil and level.is_completed
end

-- The areas the player can start next: not finished yet, with every area they
-- need finished, and with a level to start.
local function open_areas(areas)
  local open = {}
  for i, area in ipairs(areas) do
    local ready = area.start_level ~= nil and not is_finished(area)
    for _, need in ipairs(area.prereqs) do
      if areas[need] == nil or not is_finished(areas[need]) then
        ready = false
      end
    end
    open[i] = ready
  end
  return open
end

local function area_name(index)
  return trx.locale.get("general/globe_select/area_" .. index)
end

-------------------------------------------------------------------------------
-- The globe
-------------------------------------------------------------------------------

-- The globe's meshes: the sea, the markers of the areas, and the land.
local SEA_MESH = 0
local MESH_COUNT = 12
local ALL_MESHES = (1 << MESH_COUNT) - 1

-- The light the ring lit the globe in, in its world: an ambient light, and
-- three lights in front of it, up to either side and below.
local AMBIENT = { r = 32, g = 32, b = 32 }
local LIGHT_DIRS = {
  { x = 4096, y = -4096, z = 3072 },
  { x = -4096, y = -4096, z = 3072 },
  { x = 0, y = 2048, z = 3072 },
}
local WHITE = { r = 255, g = 255, b = 255 }
-- The colours, in sixteenths of a full light as the game gives them.
local function light_color(r, g, b)
  return { r = r * 255 / 4096, g = g * 255 / 4096, b = b * 255 / 4096 }
end
local SEA = light_color(0, 256, 3840)
local LAND = light_color(256, 1024, 256)

-- How many frames the marker of each area takes to brighten and fade again.
local PULSE_FRAMES = 16
-- How close the globe has to come to an area, in each axis, before it rests.
local REST_ANGLE = 128

-- How bright the markers are at the frame being drawn, from 0 to 1.
local function pulse()
  local frame = trx.signal.tick:get() - 1 + trx.ui.interpolation_rate()
  local angle = (math.floor(frame) % PULSE_FRAMES)
    * (4 * DEG_90 // PULSE_FRAMES)
  return (trx.math.sin(angle) + 1) / 2
end

-- The light of the globe: blue seas, green land, and a marker on each area
-- that pulses red where the adventure is done and green where it is not.
local function light(state)
  local bright = pulse() * 255
  local meshes = {}
  for mesh = 0, MESH_COUNT - 1 do
    meshes[mesh] = mesh == SEA_MESH and SEA or LAND
  end
  for index, area in ipairs(state.areas) do
    meshes[area.mesh] = state.open[index] and { r = 0, g = bright, b = 0 }
      or { r = bright, g = 0, b = 0 }
  end
  local lights = {}
  for i, dir in ipairs(LIGHT_DIRS) do
    lights[i] = { dir = dir, color = WHITE }
  end
  return { ambient = AMBIENT, lights = lights, meshes = meshes }
end

-- The globe draws only the markers of areas an adventure starts in.
local function mesh_mask(state)
  local mask = ALL_MESHES
  for _, area in ipairs(state.areas) do
    if area.start_level == nil then
      mask = mask & ~(1 << area.mesh)
    end
  end
  return mask
end

local wrap = trx.math.wrap_angle

-- Turns one axis an eighth of the way to its target, or onto it once close.
local function approach(current, target)
  local gap = wrap(target - current)
  if gap >= REST_ANGLE or gap <= -REST_ANGLE then
    return wrap(current + gap // 8), false
  end
  return target, true
end

-- Turns the globe a tick's worth towards the selected area. Returns whether it
-- has come to rest on it.
local function turn(state)
  local area = state.areas[state.selection]
  if area == nil then
    return true
  end
  local rest_x, rest_y, rest_z
  state.rot.x, rest_x = approach(state.rot.x, area.rot.x)
  state.rot.y, rest_y = approach(state.rot.y, area.rot.y)
  state.rot.z, rest_z = approach(state.rot.z, area.rot.z)
  return rest_x and rest_y and rest_z
end

local function paint_globe(state)
  local view = state.ring.view
  trx.ui.primitive.world_model(trx.catalog.objects.GLOBE_OPTION, {
    camera = view:camera(),
    pose = view:pose(1, {
      { turn = { x = state.rot.x, y = state.rot.y, z = state.rot.z } },
    }),
    mesh_mask = mesh_mask(state),
    light = light(state),
  })
end

local function paint(state)
  trx.ui.primitive.background(state.background, true, 0.5)
  paint_globe(state)
end

-------------------------------------------------------------------------------
-- The screen
-------------------------------------------------------------------------------

local function show_area(state)
  if state.selection == nil then
    trx.overlay.set_caption()
    return
  end
  trx.overlay.set_caption(
    trx.locale
      .get("general/inventory_ring/object_name_fmt")
      :format(area_name(state.selection))
  )
end

local function show_arrows(shown)
  trx.overlay.show_arrow(trx.overlay.Arrow.CAPTION_LEFT, shown)
  trx.overlay.show_arrow(trx.overlay.Arrow.CAPTION_RIGHT, shown)
end

local function open_count(state)
  local count = 0
  for _, open in pairs(state.open) do
    if open then
      count = count + 1
    end
  end
  return count
end

-- The next area the player can start, going one way round the globe.
local function next_open(state, direction)
  local count = #state.areas
  for offset = 1, count do
    local index = (state.selection - 1 + direction * offset) % count + 1
    if state.open[index] then
      return index
    end
  end
  return state.selection
end

local function heading()
  return trx.ui.widgets.Label({
    text = trx.locale
      .get("general/inventory_ring/heading_fmt")
      :format(trx.locale.get("general/inventory_ring/heading_adventure")),
  })
end

-- Names the area the globe is turned to, over the globe, while it is picked.
local function show_names(state, shown)
  if shown == (state.heading ~= nil) then
    return
  end
  if shown then
    show_area(state)
    show_arrows(open_count(state) > 1)
    -- The heading stands over the globe, at the top of the screen, and
    -- leaves the keys to it.
    state.heading = state.ctx:push({
      root = heading(),
      place = function(w)
        local safe = trx.ui.safe_area
        return safe.x + (safe.width - w) / 2, safe.y
      end,
    })
  else
    trx.overlay.set_caption()
    show_arrows(false)
    state.heading:close()
    state.heading = nil
  end
end

-- The player turns the globe and picks an area while the ring holds it up.
-- Returns whether they picked one.
local function choose(state, keys)
  if state.selection == nil then
    return false
  end
  if keys == nil then
    return turn(state) and false
  end
  local rested = turn(state)
  if rested then
    if keys:held(trx.input.Role.MENU_LEFT) then
      state.selection = next_open(state, -1)
    elseif keys:held(trx.input.Role.MENU_RIGHT) then
      state.selection = next_open(state, 1)
    end
  end
  show_area(state)
  return rested and keys:pressed(trx.input.Role.MENU_CONFIRM)
end

-- Starts the adventure picked, once.
local function go(state)
  if state.gone then
    return
  end
  state.gone = true
  trx.game.play_level(state.areas[state.selection].start_level.num)
end

-- Runs the ring a tick, as the inventory ring ran: what it does next, then
-- how the globe spins and the ring moves.
local function run_ring(state, keys)
  local ring = state.ring

  if ring.status == Status.OPENING and not ring.spun_in then
    trx.sound.play(trx.catalog.samples.MENU_SPININ)
    ring.spun_in = true
  elseif ring.status == Status.OPEN then
    trx.sound.play(trx.catalog.samples.MENU_CHOOSE)
    set_status(ring, Status.SELECTING, motion.Motion.SELECT)
  elseif ring.status == Status.SELECTED then
    if choose(state, keys) then
      set_status(ring, Status.CLOSING_ITEM)
    end
  elseif ring.status == Status.CLOSING_ITEM then
    set_status(ring, Status.EXITING, motion.Motion.DESELECT)
  elseif ring.status == Status.EXITING and not ring.view:is_moving() then
    set_status(ring, Status.CLOSING, motion.Motion.CLOSE)
  elseif ring.status == Status.FADING_OUT and not state.leaving then
    state.leaving = true
    state.fades:leave({
      ticks = FADE.close_ticks,
      hold = FADE.close_hold_ticks,
      on_done = function()
        ring.status = Status.DONE
        state.fades:wait(FADE.wait_ticks, function()
          go(state)
        end)
      end,
    })
  end
  show_names(state, ring.status == Status.SELECTED)

  local spin = motion.Spin.IDLE
  if
    ring.status == Status.OPENING
    or ring.status == Status.DONE
    or ring.status == Status.FADING_OUT
  then
    spin = motion.Spin.STILL
  elseif
    ring.status == Status.SELECTED
    or ring.status == Status.SELECTING
    or ring.status == Status.CLOSING_ITEM
  then
    spin = motion.Spin.HELD
  end
  if ring.view:tick(spin) and AFTER[ring.status] ~= nil then
    ring.status = AFTER[ring.status]
  end
end

-- Runs a tick of the screen with the keys the screen read, or with none on
-- the tick the screen opens on, which reads no input.
local function control(state, keys)
  state.started = true
  if state.gone then
    return
  end
  if state.ring.status == Status.DONE then
    local skip = keys ~= nil
      and (
        keys:pressed(trx.input.Role.MENU_CONFIRM)
        or keys:pressed(trx.input.Role.MENU_BACK)
      )
    if skip then
      go(state)
    end
    return
  end
  run_ring(state, keys)
end

---@param ctx trx.ui.GlobeContext
local function open(ctx)
  local areas = trx.game.globe()
  local state = {
    areas = areas,
    open = open_areas(areas),
    background = ctx.background,
    ctx = ctx,
    rot = { x = 0, y = 0, z = 0 },
    ring = open_ring(),
    fades = fade.screen(ctx),
  }
  state.fades:enter({ ticks = FADE.open_ticks })
  for index in ipairs(areas) do
    if state.open[index] then
      state.selection = index
      break
    end
  end

  -- The ring moves from the tick the screen opens on, as the inventory ring
  -- did, although the screen reads no input on that tick.
  local first = timer.after(0, function()
    if not state.started then
      control(state, nil)
    end
  end, { clock = timer.Clock.UI })
  ctx:on_end(function()
    first:cancel()
    trx.overlay.set_caption()
    show_arrows(false)
  end)
  return ctx:push({
    root = trx.ui.widgets.Custom({
      measure = function()
        return trx.ui.canvas.width, trx.ui.canvas.height
      end,
      paint = function()
        paint(state)
      end,
    }),
    place = function()
      return 0, 0
    end,
    on_input = function(_, keys)
      control(state, keys)
    end,
  })
end

function M.setup()
  trx.ui.screens.define(trx.ui.Screen.GLOBE_SELECT, open)
end

return M
