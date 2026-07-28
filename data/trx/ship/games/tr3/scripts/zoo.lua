trx.events.on_game_start(function(level)
  trx.creatures.add_ally(trx.catalog.objects.monkey)
  -- Setup shoals
  trx.items[65].properties.range = { x = 14, y = 6, z = 14 }
  trx.items[70].properties.range = { x = 14, y = 6, z = 14 }
end)
