-- Sets how the inventory ring draws a level's puzzle, key, pickup, and examine
-- items. TR4 sets these items per level.

local M = {}

-- The original game holds an inventory object at a distance and stores that
-- distance instead of a size. The ring uses its own camera, so this value is
-- the size that looks correct there. Increase it to draw every item larger.
local REFERENCE_DISTANCE = 800

-- The original game shifts an item up or down the screen in pixels, at the
-- 640 by 480 screen it was written for. Dividing that shift by the projection
-- it draws through turns it into the distance the ring moves the object by,
-- and the item's own distance cancels out. The projection is half the screen
-- width over the tangent of half the field of view.
local OG_FOCAL = (640 / 2) / math.tan(math.rad(80 / 2))

-- Turns an object to face the player. The original game's inventory shows an
-- object from the side opposite the ring, so every pose below carries this
-- half turn.
local FACING = 180

-- Takes the level's items, keyed by the object each item draws. Each item
-- stores its distance, height, rotations in degrees, pivot setting, and mesh
-- selection. A setup stays active until another level sets its own setup.
function M.apply(items)
  for object, item in pairs(items) do
    local icon = trx.inventory.icon_of(trx.catalog.objects[object])
    if icon ~= nil then
      trx.inventory.declare_ring_item({
        object_id = trx.catalog.key(trx.catalog.Context.OBJECTS, icon),
        scale = REFERENCE_DISTANCE / item.scale,
        y_offset = math.floor(
          (item.offset_y or 0) * REFERENCE_DISTANCE / OG_FOCAL + 0.5
        ),
        base_rot_x = trx.math.degrees(item.rot_x or 0),
        base_rot_y = trx.math.degrees((item.rot_y or 0) + FACING),
        base_rot_z = trx.math.degrees(item.rot_z or 0),
        draws_at_pivot = item.draws_at_pivot or false,
        meshes_sel = item.meshes or -1,
        meshes_drawn = item.meshes or -1,
      })
    end
  end
end

return M
