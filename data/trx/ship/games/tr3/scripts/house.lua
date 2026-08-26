require("common.assault")

trx.events.on_game_start(function()
  -- Setup shoals
  trx.items[55].properties.range = { x = 22, y = 2, z = 10 }
  trx.items[56].properties.range = { x = 18, y = 1, z = 10 }
  trx.items[57].properties.range = { x = 10, y = 1, z = 14 }
  trx.items[58].properties.range = { x = 18, y = 2, z = 10 }
  trx.items[59].properties.range = { x = 26, y = 2, z = 10 }
  trx.items[77].properties.range = { x = 10, y = 2, z = 18 }
  trx.items[56].properties.sprite_offset = 1
  trx.items[57].properties.sprite_offset = 1
  trx.items[59].properties.sprite_offset = 1
end)
