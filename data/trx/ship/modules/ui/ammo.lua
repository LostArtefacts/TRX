-- The kinds of ammunition a weapon takes, as the inventory shows them: the
-- boxes side by side, the shots Lara carries of each, and the kind the weapon
-- fires. The flat menu and the inventory ring both choose a kind through this
-- module, so that the two show the same thing.

local M = {}

trx.locale.declare({
  ["general/ammo/count_fmt"] = "%d x %s",
  ["general/ammo/unlimited_fmt"] = "Unlimited %s",
})

---@class common.ui.ammo.Kind
---@field kind trx.catalog.objects The kind, named by the box it arrives as.
---@field object trx.catalog.objects The icon the box is drawn as.

-- Returns the kinds of ammunition a weapon takes, in the order the inventory
-- shows them.
---@param weapon trx.weapons.Weapon
---@return common.ui.ammo.Kind[]
function M.kinds(weapon)
  local result = {}
  for _, kind in ipairs(weapon.ammo_objects) do
    result[#result + 1] = {
      kind = kind,
      object = trx.inventory.icon_of(kind) or kind,
    }
  end
  return result
end

-- Returns whether the player chooses among the kinds of a weapon, as for the
-- TR4 shotgun, grenade gun and crossbow.
---@param weapon trx.weapons.Weapon
---@return boolean
function M.is_choosable(weapon)
  return #weapon.ammo_objects > 1
end

-- Returns where the kind the weapon fires stands among its kinds.
---@param weapon trx.weapons.Weapon
---@return integer
function M.fired_index(weapon)
  local fired = trx.inventory.ammo_kind(weapon.id)
  for i, kind in ipairs(weapon.ammo_objects) do
    if kind == fired then
      return i
    end
  end
  return 1
end

-- Returns the name of a kind with the shots Lara carries of it.
---@param weapon trx.weapons.Weapon
---@param kind trx.catalog.objects
---@return string
function M.caption(weapon, kind)
  local info = trx.objects[trx.inventory.icon_of(kind) or kind]
  local name = info ~= nil and info.name or ""
  if weapon.has_infinite_ammo then
    return trx.locale.get("general/ammo/unlimited_fmt"):format(name)
  end
  local shots = trx.inventory.shots(weapon.id, kind)
  return trx.locale.get("general/ammo/count_fmt"):format(shots, name)
end

-- Returns the next kind after the one the weapon fires that Lara has shots
-- of, going back to the first after the last, or nil where there is none.
---@param weapon trx.weapons.Weapon
---@return trx.catalog.objects?
function M.next_kind(weapon)
  local kinds = weapon.ammo_objects
  local fired = M.fired_index(weapon)
  for step = 1, #kinds - 1 do
    local kind = kinds[(fired + step - 1) % #kinds + 1]
    if
      weapon.has_infinite_ammo or trx.inventory.shots(weapon.id, kind) > 0
    then
      return kind
    end
  end
  return nil
end

-- Makes a kind the one the weapon fires.
---@param weapon trx.weapons.Weapon
---@param kind trx.catalog.objects
function M.choose(weapon, kind)
  trx.inventory.set_ammo_kind(weapon.id, kind)
end

-- Says each time the player cycles a weapon on to its next kind, with the
-- weapon and the kind it fires now.
---@type trx.signal.Signal
M.cycled = trx.signal.new(nil)

-- Moves a weapon on to its next kind that Lara has shots of, as the cycle
-- ammo key does, and says so through `M.cycled`. Does nothing where there is
-- no other kind to move to.
---@param weapon trx.weapons.Weapon
function M.cycle(weapon)
  local kind = M.next_kind(weapon)
  if kind == nil then
    return
  end
  M.choose(weapon, kind)
  M.cycled:set({ weapon = weapon.id, kind = kind })
end

return M
