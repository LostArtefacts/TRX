trx.events.before_item_setup(function(level)
  -- Setup shoals
  trx.items[1].properties.range = { x = 6, y = 3, z = 30 }
  trx.items[20].properties.range = { x = 6, y = 2, z = 18 }
  trx.items[26].properties.range = { x = 6, y = 2, z = 6 }
end)
