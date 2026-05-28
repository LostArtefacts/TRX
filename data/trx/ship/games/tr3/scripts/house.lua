trx.events.before_item_setup(function(level)
  -- Setup shoals
  trx.items[56].properties.range = { x = 22, y = 2, z = 10 }
  trx.items[57].properties.range = { x = 18, y = 1, z = 10 }
  trx.items[58].properties.range = { x = 10, y = 1, z = 14 }
  trx.items[59].properties.range = { x = 18, y = 2, z = 10 }
  trx.items[60].properties.range = { x = 26, y = 2, z = 10 }
  trx.items[78].properties.range = { x = 10, y = 2, z = 18 }
  trx.items[57].properties.use_default_uv = false
  trx.items[58].properties.use_default_uv = false
  trx.items[60].properties.use_default_uv = false
end)
