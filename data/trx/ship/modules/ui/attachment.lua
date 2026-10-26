-- How the inventory shows a weapon that carries its attachment, such as the
-- TR4 revolver with the laser sight: its name, and the meshes it is drawn
-- with. The flat menu and the inventory ring both show it through this
-- module, so that the two show the same thing.

local M = {}

trx.locale.declare({
  ["general/inventory/attached_fmt"] = "%s + %s",
})

-- Returns the weapon an inventory entry draws that carries its attachment,
-- or nil for any other entry.
---@param object trx.catalog.objects
---@return trx.weapons.Weapon?
local function attached_weapon_of(object)
  for _, weapon in ipairs(trx.weapons.all) do
    if
      weapon.object ~= nil
      and weapon.attachment ~= nil
      and trx.inventory.icon_of(weapon.object) == object
      and trx.inventory.has_attachment(weapon.id)
    then
      return weapon
    end
  end
  return nil
end

-- Returns the name of an inventory entry, with the name of its attachment
-- where it is a weapon that carries one.
---@param object trx.catalog.objects
---@param name string|trx.locale.Text The entry's own name.
---@return string|trx.locale.Text
function M.name_of(object, name)
  local weapon = attached_weapon_of(object)
  if weapon == nil then
    return name
  end
  return trx.locale.text(
    "general/inventory/attached_fmt",
    name,
    trx.locale.object_name(weapon.attachment.object)
  )
end

-- Returns the meshes an inventory entry is drawn with where it is a weapon
-- that carries its attachment, or nil to draw it as usual.
---@param object trx.catalog.objects
---@return integer?
function M.meshes_of(object)
  local weapon = attached_weapon_of(object)
  if weapon == nil or weapon.attachment.inventory_meshes < 0 then
    return nil
  end
  return weapon.attachment.inventory_meshes
end

return M
