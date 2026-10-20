-- The fade of the TR4 compass needle at true north, which the inventory ring
-- and the flat menu both draw.
--
-- Usage:
--   local compass = require("common.ui.compass")
--   opts.mesh_opacity = compass.mesh_opacity()

local M = {}

-- The opacity the TR4 compass draws its needle with while Lara faces true
-- north, as the original does, and how near north she has to face.
local NORTH_NEEDLE_OPACITY = 192 / 255
local NORTH_REACH = 48

-- Returns whether Lara faces true north, as near as the TR4 compass shows it.
local function faces_north()
  local lara = trx.lara ~= nil and trx.lara.item or nil
  if lara == nil then
    return false
  end
  local yaw = trx.math.wrap_angle(lara.rot.y)
  return yaw > -NORTH_REACH and yaw <= NORTH_REACH
end

-- The opacity of each faded compass mesh, for a world model's mesh_opacity.
---@return table<integer, number>?
function M.mesh_opacity()
  if trx.game.version == 4 and faces_north() then
    return { [1] = NORTH_NEEDLE_OPACITY }
  end
  return nil
end

return M
