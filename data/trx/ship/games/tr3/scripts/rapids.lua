trx.events.before_item_setup(function(level)
  -- Setup shoals
  trx.items[16].properties.range = { x = 6, y = 5, z = 10 }
  trx.items[52].properties.range = { x = 18, y = 8, z = 18 }

  trx.objects.flame_emitter_side.properties.interval = 4
end)
