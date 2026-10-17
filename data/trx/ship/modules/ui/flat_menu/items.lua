-- How the flat menu draws each entry, as the original TR4 draws the items of
-- its inventory. Each row names an item and gives the pixels it stands below
-- its point on the 480-line screen, how far from the camera it is held, and
-- how it is turned: about the X, Y and Z axes. An item spins while it is the
-- one picked unless it stands still, and it can be drawn about its pivot
-- rather than at the offset of its frame. An item that opens is drawn open,
-- in the frame the inventory ring rests it on once open. The rows stand in
-- the order the original shows the items in.

local M = {}

-- The original holds every item it has no row for this far away, and a little
-- below its point.
local DEFAULT =
  { y_offset = 8, distance = 1200, rot = { x = 0, y = 0, z = 0 } }

-- stylua: ignore
---@type table[]
local ROWS = {
  { "pistols_item", 6, 1000, -21172, 16384, -17088 },
  { "uzis_item", -4, 1000, 24576, 16384, 16384 },
  { "uzis_ammo_item", 5, 700, 5384, 0, 0 },
  { "revolver_item", 0, 800, 10922, 16384, 15298, meshes = 1 },
  { "revolver_ammo_item", 0, 700, -3000, 16384, 0 },
  { "shotgun_item", -6, 640, -16384, -32768, 8192, meshes = 1 },
  { "shotgun_ammo_item", 0, 500, 0, 16384, 0 },
  { "shotgun_ammo_2_item", 0, 500, 0, 16384, 0 },
  { "grenade_gun_item", 0, 800, 0, 16384, 11980 },
  { "grenade_ammo_item", 3, 800, 0, 16384, 0 },
  { "grenade_ammo_2_item", 3, 800, 0, 16384, 0 },
  { "grenade_ammo_3_item", 3, 800, 0, 16384, 0 },
  { "crossbow_item", 0, 768, 6144, 8192, 0, meshes = 1 },
  { "crossbow_ammo_1_item", 0, 1100, 0, 16384, 0 },
  { "crossbow_ammo_2_item", 0, 1100, 0, 16384, 0 },
  { "crossbow_ammo_3_item", 0, 1100, 0, 16384, 0 },
  { "pistols_ammo_item", 4, 700, 16384, 0, 0 },
  { "lasersight_item", 2, 700, 2000, 16384, 0 },
  { "binoculars_item", -1, 700, 2000, 4096, 0 },
  { "flares_box_item", 2, 1100, 0, 16384, 0 },
  { "small_medipack_item", 0, 512, 20480, 0, 0 },
  { "large_medipack_item", 2, 800, 0, 0, 0 },
  { "waterskin_1_empty", 2, 1600, -14000, 0, 0 },
  { "waterskin_1_1", 2, 1600, -14000, 0, 0 },
  { "waterskin_1_2", 2, 1600, -14000, 0, 0 },
  { "waterskin_1_3", 2, 1600, -14000, 0, 0 },
  { "waterskin_2_empty", 2, 1600, -14000, 0, 0 },
  { "waterskin_2_1", 2, 1600, -14000, 0, 0 },
  { "waterskin_2_2", 2, 1600, -14000, 0, 0 },
  { "waterskin_2_3", 2, 1600, -14000, 0, 0 },
  { "waterskin_2_4", 2, 1600, -14000, 0, 0 },
  { "waterskin_2_5", 2, 1600, -14000, 0, 0 },
  { "crowbar_item", 4, 1900, 16384, 0, 0 },
  { "puzzle_item_1", 14, 1200, 0, 0, 0 },
  { "puzzle_item_2", 14, 600, -16384, 0, 0 },
  { "puzzle_item_3", 8, 1200, 0, 0, 0 },
  { "puzzle_item_4", 8, 1200, 0, 0, 0 },
  { "puzzle_item_5", 8, 1200, 0, 0, 0 },
  { "puzzle_item_6", 8, 1200, 0, 0, 0 },
  { "puzzle_item_7", 8, 1200, 0, 0, 0 },
  { "puzzle_item_8", 8, 1200, 0, 0, 0 },
  { "puzzle_item_9", 8, 1200, 0, 0, 0 },
  { "puzzle_item_10", 8, 1200, 0, 0, 0 },
  { "puzzle_item_11", 8, 1200, 0, 0, 0 },
  { "puzzle_item_12", 8, 1200, 0, 0, 0 },
  { "puzzle_item_1_combo_1", 18, 1200, 0, 0, 0 },
  { "puzzle_item_1_combo_2", 18, 1200, 0, 0, 0 },
  { "puzzle_item_2_combo_1", 8, 1200, 0, 0, 0 },
  { "puzzle_item_2_combo_2", 8, 1200, 0, 0, 0 },
  { "puzzle_item_3_combo_1", 8, 1200, 0, 0, 0 },
  { "puzzle_item_3_combo_2", 8, 1200, 0, 0, 0 },
  { "puzzle_item_4_combo_1", 8, 1200, 0, 0, 0 },
  { "puzzle_item_4_combo_2", 8, 1200, 0, 0, 0 },
  { "puzzle_item_5_combo_1", 8, 1200, 0, 0, 0 },
  { "puzzle_item_5_combo_2", 8, 1200, 0, 0, 0 },
  { "puzzle_item_6_combo_1", 8, 1200, 0, 0, 0 },
  { "puzzle_item_6_combo_2", 8, 1200, 0, 0, 0 },
  { "puzzle_item_7_combo_1", 8, 1200, 0, 0, 0 },
  { "puzzle_item_7_combo_2", 8, 1200, 0, 0, 0 },
  { "puzzle_item_8_combo_1", 8, 1200, 0, 0, 0 },
  { "puzzle_item_8_combo_2", 8, 1200, 0, 0, 0 },
  { "key_item_1", 14, 1200, 0, 0, 0 },
  { "key_item_2", 8, 1200, 0, 0, 0 },
  { "key_item_3", 8, 1200, 0, 0, 0 },
  { "key_item_4", 8, 1200, 0, 0, 0 },
  { "key_item_5", 8, 1200, 0, 0, 0 },
  { "key_item_6", 8, 1200, 0, 0, 0 },
  { "key_item_7", 8, 1200, 0, 0, 0 },
  { "key_item_8", 8, 1200, 0, 0, 0 },
  { "key_item_9", 8, 1200, 0, 0, 0 },
  { "key_item_10", 8, 1200, 0, 0, 0 },
  { "key_item_11", 4, 1200, 0, 0, 0 },
  { "key_item_12", 8, 1200, 0, 0, 0 },
  { "key_item_1_combo_1", 18, 1200, 0, 0, 0 },
  { "key_item_1_combo_2", 18, 1200, 0, 0, 0 },
  { "key_item_2_combo_1", 8, 1200, 0, 0, 0 },
  { "key_item_2_combo_2", 8, 1200, 0, 0, 0 },
  { "key_item_3_combo_1", 8, 1200, 0, 0, 0 },
  { "key_item_3_combo_2", 8, 1200, 0, 0, 0 },
  { "key_item_4_combo_1", 8, 1200, 0, 0, 0 },
  { "key_item_4_combo_2", 8, 1200, 0, 0, 0 },
  { "key_item_5_combo_1", 8, 1200, 0, 0, 0 },
  { "key_item_5_combo_2", 8, 1200, 0, 0, 0 },
  { "key_item_6_combo_1", 8, 1200, 0, 0, 0 },
  { "key_item_6_combo_2", 8, 1200, 0, 0, 0 },
  { "key_item_7_combo_1", 8, 1200, 0, 0, 0 },
  { "key_item_7_combo_2", 8, 1200, 0, 0, 0 },
  { "key_item_8_combo_1", 8, 1200, 0, 0, 0 },
  { "key_item_8_combo_2", 8, 1200, 0, 0, 0 },
  { "pickup_item_1", 14, 1200, 0, 0, 0 },
  { "pickup_item_2", 8, 1200, 0, 0, 0 },
  { "pickup_item_3", 8, 1200, 0, 0, 0 },
  { "pickup_item_4", 8, 1200, 0, 0, 0 },
  { "pickup_item_1_combo_1", 14, 1200, 0, 0, 0 },
  { "pickup_item_1_combo_2", 14, 1200, 0, 0, 0 },
  { "pickup_item_2_combo_1", 14, 1200, 0, 0, 0 },
  { "pickup_item_2_combo_2", 14, 1200, 0, 0, 0 },
  { "pickup_item_3_combo_1", 14, 1200, 0, 0, 0 },
  { "pickup_item_3_combo_2", 14, 1200, 0, 0, 0 },
  { "pickup_item_4_combo_1", 14, 1200, 0, 0, 0 },
  { "pickup_item_4_combo_2", 14, 1200, 0, 0, 0 },
  { "quest_item_1", 0, 1200, -16384, -32768, 0 },
  { "quest_item_2", 0, 1024, 0, -32768, 0 },
  { "quest_item_3", 0, 1280, 0, -16384, -16384 },
  { "quest_item_4", 0, 1280, 0, 16384, -16384 },
  { "quest_item_5", 0, 768, 0, 0, 8192 },
  { "quest_item_6", 0, 768, 0, -32768, 8192 },
  { "examine_item_1", 4, 1300, 16384, 0, 0 },
  { "examine_item_2", 14, 1200, 16384, 0, 0 },
  { "examine_item_3", 14, 1200, 16384, 0, 0 },
  -- The TR4 compass that TRX injects lies level and centred on its origin,
  -- where the original model stood below it. This row places it where the
  -- original shows its compass, measured against the original's screen, in
  -- place of the original's -14 and 600.
  { "compass_option", 37, 650, 14000, 0, 0, still = true, opens = true },
  { "load_option", 2, 4000, 0, 0, 0 },
  { "save_option", 2, 4000, 0, 0, 0 },
}

-- The rows that differ in TR1, whose compass opens its lid the other way
-- from the TR4 one and turns round to show its face.
-- stylua: ignore
---@type table[]
local TR1_ROWS = {
  { "compass_option", -14, 600, -14000, 32768, 0, still = true, opens = true },
}

---@class common.ui.flat_menu.Item
---@field y_offset integer
---@field distance integer
---@field rot { x: integer, y: integer, z: integer }
---@field still? boolean
---@field opens? boolean
---@field at_pivot? boolean
---@field meshes? integer

---@type table<integer, common.ui.flat_menu.Item>?
local by_icon = nil

-- Declarations a level makes on top of the rows, by the icon each draws.
---@type table<integer, table>
local declared = {}

local function icon_of(object)
  return trx.inventory.icon_of(object) or object
end

local function add(rows)
  for _, row in ipairs(rows) do
    local object = trx.catalog.objects[row[1]]
    if object ~= nil then
      by_icon[icon_of(object)] = {
        y_offset = row[2],
        distance = row[3],
        rot = { x = row[4], y = row[5], z = row[6] },
        still = row.still,
        opens = row.opens,
        at_pivot = row.at_pivot,
        meshes = row.meshes,
      }
    end
  end
end

local function build()
  by_icon = {}
  add(ROWS)
  if trx.game.version == 1 then
    add(TR1_ROWS)
  end
end

-- Returns how the flat menu draws an entry, by the icon it is drawn as.
---@param icon integer
---@return common.ui.flat_menu.Item
function M.of(icon)
  if by_icon == nil then
    build()
  end
  local item = by_icon[icon] or DEFAULT
  local over = declared[icon]
  if over == nil then
    return item
  end
  local result = {}
  for key, value in pairs(item) do
    result[key] = value
  end
  for key, value in pairs(over) do
    result[key] = value
  end
  return result --[[@as common.ui.flat_menu.Item]]
end

-- Changes how an entry is drawn, by the icon it is drawn as. A level sets its
-- own puzzle items, keys and pickups this way, as the original TR4 script
-- does. The change lasts until the next level declares the icon again.
---@param icon integer
---@param item table The fields of `common.ui.flat_menu.Item` to change.
function M.declare(icon, item)
  declared[icon] = item
end

-- Returns where an item sits among the rows, so that the menu orders what
-- Lara carries as the original does, or nil for an item with no row.
---@param icon integer
---@return integer?
function M.order(icon)
  if by_icon == nil then
    build()
  end
  for i, row in ipairs(ROWS) do
    local object = trx.catalog.objects[row[1]]
    if object ~= nil and icon_of(object) == icon then
      return i
    end
  end
  return nil
end

return M
