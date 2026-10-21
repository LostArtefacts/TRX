-- A row of entries across the screen, as the original TR4 lays out its
-- inventory. The entry in the middle is the one picked. A turn slides the row
-- by one place over eight ticks, and the entries at its ends fade as they come
-- and go.

local entries = require("common.ui.ring.entries")
local items = require("common.ui.flat_menu.items")

local M = {}

-- How far the row slides each tick, as a share of one place.
local SLIDE_STEP = 1 / 8
-- How far the picked entry spins each tick, and how fast the others turn back.
local SPIN = 1022
local SPIN_BACK_MIN = 1022
local SPIN_BACK_MAX = 16384
-- How bright an entry is drawn: brighter while it is the one picked, and
-- changing by a step each tick.
local BRIGHT_PICKED = 160
local BRIGHT_REST = 32
local BRIGHT_STEP = 16
-- How bright an entry of a fixed row is drawn. The original sets it at once,
-- with no steps.
local FIXED_BRIGHT_PICKED = 127
local FIXED_BRIGHT_REST = 47
-- How long the row takes to fade in or out as a whole.
local FADE_TICKS = 4
-- The original's field of view, and the share of the screen's height that
-- its camera's focal length spans on the 4:3 screen it was written for.
local FOV = 80
local FOCAL_SHARE = 320 / 480 / math.tan(math.rad(FOV / 2))

local function wrap(angle)
  return trx.math.wrap_angle(angle)
end

-- Turns an angle a step back towards zero, as the original turns an entry
-- that is no longer picked back to face the player. The angle is a 16-bit
-- value, as the original keeps it.
local function spin_back(angle)
  local value = angle % 65536
  if value == 0 then
    return 0
  end
  if value <= 32768 then
    local by = value
    if by < SPIN_BACK_MIN then
      value = SPIN_BACK_MIN
    elseif by > SPIN_BACK_MAX then
      by = SPIN_BACK_MAX
    end
    value = (value - (by >> 3)) % 65536
    if value > 32768 then
      value = 0
    end
  else
    local by = 65536 - value
    if by < SPIN_BACK_MIN then
      value = SPIN_BACK_MIN
    elseif by > SPIN_BACK_MAX then
      by = SPIN_BACK_MAX
    end
    value = (value + (by >> 3)) % 65536
    if value < 32768 then
      value = 0
    end
  end
  return value
end

-- A value that steps once a tick and is read between the last two steps
-- while the interface is painted.
local function stepped(value)
  return { last = value, now = value }
end

local function step(value, now)
  value.last = value.now
  value.now = now
end

-- Steps an angle by the shortest way to its new value, so that it is read
-- smoothly as it passes a full turn.
local function step_angle(value, now)
  step(value, value.now + wrap(now - value.now))
end

local function read(value)
  return value.last + (value.now - value.last) * trx.ui.interpolation_rate()
end

---@class common.ui.flat_menu.Strip
---@field list table[] The entries, each with its `object`, as the inventory reports them.
---@field current integer The entry picked, counted from 1.
---@field row number Where the row's middle stands, as a share of the screen's height.
---@field fixed boolean Whether every entry stands in place, centred as a group, rather than sliding past the middle.
---@field position table The place the row's middle stands at, stepped each tick, counting entries from 0 without wrapping.
---@field target number The place the row slides to.
---@field fade trx.ui.Tween How visible the row is as a whole, from 0 to 1.
---@field anims table[] For each entry, its turn and its brightness.
---@field on_change? fun(strip: common.ui.flat_menu.Strip)
local Strip = {}
Strip.__index = Strip

-- Makes a row of entries. A fixed row stands in place, as the original's row
-- of ammunition does, and picking an entry spins it where it stands.
---@param list table[]
---@param row number Where the row stands, as a share of the screen's height.
---@param opts? { fixed?: boolean }
---@return common.ui.flat_menu.Strip
function M.new(list, row, opts)
  local self = setmetatable({
    list = list,
    current = 1,
    row = row,
    fixed = opts ~= nil and opts.fixed == true,
    position = stepped(0),
    target = 0,
    fade = trx.ui.tween(1),
    anims = {},
  }, Strip)
  for i in ipairs(list) do
    self.anims[i] = {
      yrot = stepped(0),
      bright = stepped(self.fixed and FIXED_BRIGHT_REST or BRIGHT_REST),
    }
  end
  return self
end

-- Returns the entry at a place of the row.
function Strip:index_at(place)
  return place % #self.list + 1
end

-- The entry picked.
function Strip:picked()
  return self.list[self.current]
end

-- Picks the entry at a place of the list, counted from 1, at once.
function Strip:pick_at(index)
  self.current = index
  self.target = index - 1
  self.position = stepped(index - 1)
end

-- Picks the entry that draws an object, where the row holds one, at once.
function Strip:pick(object)
  for i, entry in ipairs(self.list) do
    if entry.object == object then
      self:pick_at(i)
      return true
    end
  end
  return false
end

function Strip:is_sliding()
  return self.position.now ~= self.target
end

-- Slides the row towards the previous entry, by 1, or the next one, by -1. A
-- turn asked for while the row slides follows it, one place ahead at most.
---@return boolean # Whether the row takes the turn.
function Strip:turn(direction)
  if #self.list <= 1 then
    return false
  end
  local target = self.target - direction
  if math.abs(target - self.position.now) > 1 then
    return false
  end
  self.target = target
  return true
end

-- Fades the row in, or out.
---@param ticks? integer How long the fade takes.
function Strip:show(shown, ticks)
  self.fade:to(shown and 1 or 0, ticks or FADE_TICKS)
end

function Strip:is_fading()
  return self.fade:is_moving()
end

-- Runs a tick: the slide, the spins and the brightness of each entry.
function Strip:tick()
  local position = self.position.now
  if position < self.target then
    position = math.min(position + SLIDE_STEP, self.target)
  elseif position > self.target then
    position = math.max(position - SLIDE_STEP, self.target)
  end
  step(self.position, position)

  local settled = position == self.target
  local picked = self:index_at(self.target)
  if settled and picked ~= self.current then
    self.current = picked
    if self.on_change ~= nil then
      self.on_change(self)
    end
  end

  -- The entry the row slides to brightens while it comes.
  for i, anim in ipairs(self.anims) do
    local item = items.of(self.list[i].object)
    local yrot = anim.yrot.now % 65536
    if i == picked and settled then
      if not item.still then
        yrot = yrot + SPIN
      end
    else
      yrot = spin_back(yrot)
    end
    step_angle(anim.yrot, yrot)

    local bright = anim.bright.now
    if self.fixed then
      bright = i == picked and FIXED_BRIGHT_PICKED or FIXED_BRIGHT_REST
    elseif i == picked then
      bright = math.min(bright + BRIGHT_STEP, BRIGHT_PICKED)
    else
      bright = math.max(bright - BRIGHT_STEP, BRIGHT_REST)
    end
    step(anim.bright, bright)
  end
end

-- How many places from the middle the row shows an entry, as the original
-- shows one entry, three or five depending on how many there are.
local function reach(count)
  if count <= 2 then
    return 0
  elseif count <= 4 then
    return 1
  end
  return 2
end

-- The field of view that keeps an entry as large against the height of the
-- screen as the original draws it on a 4:3 screen.
local function fov()
  local aspect = trx.ui.canvas.width / trx.ui.canvas.height
  local half = math.atan(math.min(aspect, 1.6) / (2 * FOCAL_SHARE))
  return math.floor(math.deg(2 * half) * trx.math.DEG_1 + 0.5)
end

-- Draws one entry with its centre at a point of the canvas.
---@param object integer The icon to draw.
---@param x number
---@param y number
---@param opts { xrot?: number, yrot?: number, zoom?: number, bright?: number, opacity?: number, bones?: trx.ui.BoneTurn[], mesh_opacity?: table<integer, number> }
function M.draw(object, x, y, opts)
  local item = items.of(object)
  local spec = item.opens and entries.item(object) or nil
  local h = trx.ui.canvas.height
  local bright = (opts.bright or 127) * 255 / 128
  trx.ui.primitive.world_model(object, {
    camera = {
      pos = { x = 0, y = 0, z = 0 },
      target = { x = 0, y = 0, z = 1024 },
      fov = fov(),
      center = { x = x, y = y + item.y_offset * h / 480 },
    },
    pose = {
      {
        move = {
          x = 0,
          y = 0,
          z = math.floor(item.distance / (opts.zoom or 1)),
        },
      },
      {
        turn = {
          x = wrap(item.rot.x + (opts.xrot or 0)),
          y = wrap(item.rot.y + (opts.yrot or 0)),
          z = wrap(item.rot.z),
        },
      },
    },
    frame = spec ~= nil and spec.open_frame or nil,
    at_pivot = item.at_pivot,
    bones = opts.bones,
    mesh_mask = item.meshes,
    mesh_opacity = opts.mesh_opacity,
    opacity = opts.opacity or 1,
    light = { ambient = { r = bright, g = bright, b = bright } },
  })
end

-- The width between two entries: a quarter of the original's 4:3 screen.
function M.spacing()
  return trx.ui.canvas.height * 4 / 3 / 4
end

-- Draws the row.
function Strip:paint()
  local count = #self.list
  local fade = self.fade:get()
  if count == 0 or fade <= 0 then
    return
  end
  local w, h = trx.ui.canvas.width, trx.ui.canvas.height
  if self.fixed then
    local spacing = M.spacing()
    for i, entry in ipairs(self.list) do
      local anim = self.anims[i]
      M.draw(
        entry.object,
        w / 2 + (i - (count + 1) / 2) * spacing,
        h * self.row,
        {
          yrot = read(anim.yrot),
          bright = read(anim.bright),
          opacity = fade,
        }
      )
    end
    return
  end
  local position = read(self.position)
  local far = reach(count) + 1
  local spacing = M.spacing()
  local y = h * self.row
  for place = math.floor(position) - far, math.ceil(position) + far do
    local offset = place - position
    local opacity = math.min(math.max(far - math.abs(offset), 0), 1)
    if opacity > 0 then
      local index = self:index_at(place)
      local anim = self.anims[index]
      M.draw(self.list[index].object, w / 2 + offset * spacing, y, {
        yrot = read(anim.yrot),
        bright = read(anim.bright),
        opacity = opacity * fade,
      })
    end
  end
end

return M
