-- Declares the inventory-ring items from the game's ring file.

local found = trx.path.resolve("common_config", "inv_ring.json5")
local specs = found ~= nil and trx.json.read_file(found) or nil
if specs == nil then
  return
end

for _, spec in ipairs(specs) do
  trx.inventory.declare_ring_item(spec)
end
