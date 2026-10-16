-- The entries the inventory ring draws: what each is declared with, and which
-- of the rings each goes on.
--
-- An entry is declared once for every game from `inv_ring.json5`, and a level
-- script may declare it again to draw it differently. A later declaration
-- changes only the fields it names, and stays for the rest of the game.
--
-- Usage:
--   local entries = require("common.ui.ring.entries")
--   entries.declare({ object_id = "dash:bfg_item", inv_pos = 20 })
--   for _, entry in ipairs(entries.contents(entries.Ring.MAIN, mode)) do ...

local M = {}

local objects = trx.catalog.objects

-- The rings, by where the entries on each are declared to sit: the main ring
-- counts from 0, the keys from 100 and the menu from 200.
M.Ring = {
  MAIN = "main",
  KEYS = "keys",
  OPTION = "option",
}

-- The most a ring counts of one entry.
local MAX_COUNT = 999999

-- The fields an entry is declared with, and what each falls back to.
local DEFAULTS = {
  frames_total = 1,
  open_frame = 0,
  anim_direction = 1,
  anim_speed = 0,
  x_rot_pt_sel = 0,
  x_rot_sel = 0,
  x_rot_nosel = 0,
  y_rot_sel = 0,
  y_trans_sel = 0,
  z_trans_sel = 0,
  scale = 1,
  y_offset = 0,
  base_rot_x = 0,
  base_rot_y = 0,
  base_rot_z = 0,
  draws_at_pivot = false,
  meshes_sel = -1,
  meshes_drawn = -1,
  inv_pos = 0,
}

-- What each entry is declared with, by its object.
---@type table<integer, table>
local declared = {}

---@class common.ui.ring.Declaration
---@field object_id string The object the entry draws, by its catalog key.
---@field frames_total? integer How many frames its animation holds.
---@field open_frame? integer The frame it rests on once open.
---@field anim_direction? integer Which way its animation runs as it opens.
---@field anim_speed? integer How many steps each frame of its animation holds past the first. `0` by default.
---@field x_rot_pt_sel? integer How far it tilts towards the camera when picked.
---@field x_rot_sel? integer How far it pitches when picked.
---@field x_rot_nosel? integer How far it pitches on the ring.
---@field y_rot_sel? integer Which way it faces when picked.
---@field y_trans_sel? integer How far it rises when picked.
---@field z_trans_sel? integer How far it comes forward when picked.
---@field scale? number How large it is drawn, with `1` for its own size.
---@field y_offset? integer How far it sits below where the ring puts it.
---@field base_rot_x? integer How far it pitches wherever the ring puts it.
---@field base_rot_y? integer Which way it faces wherever the ring puts it.
---@field base_rot_z? integer How far it rolls wherever the ring puts it.
---@field draws_at_pivot? boolean Whether it is drawn about its pivot, for a model whose frame stands it far from where the ring puts it.
---@field meshes_sel? integer Which of its meshes show, one bit each.
---@field meshes_drawn? integer Which of its meshes show at first, one bit each.
---@field inv_pos? integer Where it sits: the main ring from 0, the keys from 100, and the menu from 200.

-- Declares how the ring draws an entry, or changes the fields named of one
-- declared before.
---@param spec common.ui.ring.Declaration
function M.declare(spec)
  if type(spec) ~= "table" then
    error("a ring entry is declared with a table", 2)
  end
  local object = objects[spec.object_id]
  if object == nil then
    error(("unknown object '%s'"):format(tostring(spec.object_id)), 2)
  end
  local entry = declared[object]
  if entry == nil then
    ---@type table<string, any>
    entry = {}
    for key, value in pairs(DEFAULTS) do
      entry[key] = value
    end
    entry.object = object
    declared[object] = entry
  end
  for key in pairs(DEFAULTS) do
    if spec[key] ~= nil then
      entry[key] = spec[key]
    end
  end
end

-- Returns what an entry is declared with. The passport rests on the ring as
-- its closed cover, which takes what the open passport is declared with
-- unless the cover is declared itself.
local function declaration_of(object)
  local entry = declared[object]
  if entry == nil and object == objects.PASSPORT_CLOSED then
    entry = declared[objects.PASSPORT_OPTION]
  end
  return entry
end

-- Returns a copy of what an entry is declared with, or nil for one that is
-- not declared.
---@param object integer
---@return table?
function M.item(object)
  local entry = declaration_of(object)
  if entry == nil then
    return nil
  end
  local copy = {}
  for key, value in pairs(entry) do
    copy[key] = value
  end
  return copy
end

-- Returns the ring an entry goes on, or nil for one on none of them, such as
-- the globe.
local function ring_of(entry)
  if entry.inv_pos < 100 then
    return M.Ring.MAIN
  elseif entry.inv_pos < 200 then
    return M.Ring.KEYS
  elseif entry.inv_pos < 300 then
    return M.Ring.OPTION
  end
  return nil
end

-- Returns whether the player's settings leave an entry off the rings while it
-- stays in the inventory.
local function is_hidden(object)
  if object == objects.BINOCULARS_OPTION then
    local ok, enabled = pcall(trx.config.get, "gameplay.enable_binoculars")
    return not (ok and enabled)
  end
  return false
end

-- The entries of the menu ring, in the order each game shows them.
local function menu(mode)
  local list = { objects.PASSPORT_CLOSED }
  if trx.game.version == 1 then
    list[#list + 1] = objects.CONTROLS_OPTION
    list[#list + 1] = objects.SOUND_OPTION
    list[#list + 1] = objects.GRAPHICS_OPTION
  else
    list[#list + 1] = objects.GRAPHICS_OPTION
    list[#list + 1] = objects.CONTROLS_OPTION
    list[#list + 1] = objects.SOUND_OPTION
  end
  list[#list + 1] = objects.PDA_OPTION
  if mode == trx.ui.MenuMode.TITLE and trx.game.gym ~= nil then
    list[#list + 1] = objects.PHOTO_OPTION
  end
  return list
end

-- An entry of a ring: its object, how many of it the ring counts, and what
-- it is declared with.
---@class common.ui.ring.Content
---@field object integer
---@field count integer
---@field spec table

-- Returns the entries a ring holds, in the order they are declared to sit
-- in: the menu for the mode the ring opened in, or what Lara carries.
---@param ring string One of `M.Ring`.
---@param mode integer The `trx.ui.MenuMode` the ring opened in.
---@return common.ui.ring.Content[]
function M.contents(ring, mode)
  local result = {}
  local function add(object, count)
    local entry = declaration_of(object)
    if entry == nil or ring_of(entry) ~= ring or is_hidden(object) then
      return
    end
    result[#result + 1] = {
      object = object,
      count = math.min(count, MAX_COUNT),
      spec = M.item(object),
    }
  end

  if ring == M.Ring.OPTION then
    for _, object in ipairs(menu(mode)) do
      add(object, 1)
    end
  else
    for i = 1, trx.inventory.entry_count() do
      local entry = trx.inventory.entry_at(i)
      if entry ~= nil then
        add(entry.object, entry.count)
      end
    end
  end
  -- Entries that sit in the same place keep the order they were added in:
  -- the menu in each game's own order, and what Lara carries in hers.
  local order = {}
  for i, content in ipairs(result) do
    order[content] = i
  end
  table.sort(result, function(a, b)
    if a.spec.inv_pos ~= b.spec.inv_pos then
      return a.spec.inv_pos < b.spec.inv_pos
    end
    return order[a] < order[b]
  end)
  return result
end

-- Returns whether a ring has anything on it. The menu ring may also be locked
-- out by the game.
---@param ring string One of `M.Ring`.
---@param mode integer The `trx.ui.MenuMode` the ring opened in.
---@return boolean
function M.is_available(ring, mode)
  if ring == M.Ring.OPTION then
    local ok, locked = pcall(trx.config.get, "flow.lockout_option_ring")
    if ok and locked then
      return false
    end
  end
  return #M.contents(ring, mode) > 0
end

return M
