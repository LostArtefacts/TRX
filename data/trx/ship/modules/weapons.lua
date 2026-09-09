-- Declares the weapons from the game's weapons file. Unsupported weapons,
-- such as those fixed to vehicles, keep their settings but are not handled.

local found = trx.path.resolve("common_config", "weapons.json5")
local specs = found ~= nil and trx.json.read_file(found) or nil
if specs == nil then
  return
end

for key, spec in pairs(specs) do
  local weapon = trx.catalog.weapons[key]
  if weapon == nil then
    trx.log.warning(("unknown weapon '%s'"):format(key))
  else
    trx.weapons.declare(weapon, spec)
  end
end
