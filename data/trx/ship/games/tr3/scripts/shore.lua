trx.events.on_game_start(function(level)
  -- Setup shoals
  trx.items[8].properties.range = { x = 14, y = 6, z = 22 }
  trx.items[12].properties.range = { x = 14, y = 6, z = 14 }
  trx.items[14].properties.range = { x = 22, y = 8, z = 6 }
  trx.items[12].properties.sprite_offset = 1
  trx.items[14].properties.sprite_offset = 1
end)
