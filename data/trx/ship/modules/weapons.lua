-- Declares the weapons from the game's weapons file. Unsupported weapons,
-- such as those fixed to vehicles, keep their settings but are not handled.

local found = trx.path.resolve("common_config", "weapons.json5")
if found == nil then
  error("weapons.json5 is missing; the game directory is incomplete")
end

local specs = trx.json.read_file(found)

for key, spec in pairs(specs) do
  local weapon = trx.catalog.weapons[key]
  if weapon == nil then
    trx.log.warning(("unknown weapon '%s'"):format(key))
  else
    trx.weapons.declare(weapon, spec)
  end
end
