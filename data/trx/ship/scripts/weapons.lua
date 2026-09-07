-- Declares the weapons listed in the game's weapons file. Unsupported kinds,
-- such as weapons fixed to vehicles, keep their settings but are not handled
-- by the engine.

local found = trx.path.resolve("common_config", "weapons.json5")
local specs = found ~= nil and trx.json.read_file(found) or nil
if specs == nil then
  return
end

for key, spec in pairs(specs) do
  local weapon = trx.catalog.weapons[key]
  if weapon == nil then
    trx.log.warning(("the game does not define weapon '%s'"):format(key))
  else
    trx.weapons.declare(weapon, spec)
  end
end
