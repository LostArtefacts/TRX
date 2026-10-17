-- Declares the inventory-ring items from the game's ring file.

local ring = require("common.ui.ring")

local found = trx.path.resolve("common_config", "inv_ring.json5")
if found == nil then
  error("inv_ring.json5 is missing; the game directory is incomplete")
end

for _, spec in ipairs(trx.json.read_file(found)) do
  ring.declare_item(spec)
end
