trx.events.before_item_setup(function(level)
  -- Setup shoals
  trx.items[9].properties.range = { x = 14, y = 6, z = 22 }
  trx.items[13].properties.range = { x = 14, y = 6, z = 14 }
  trx.items[15].properties.range = { x = 22, y = 8, z = 6 }
  trx.items[13].properties.sprite_offset = 1
  trx.items[15].properties.sprite_offset = 1
end)
