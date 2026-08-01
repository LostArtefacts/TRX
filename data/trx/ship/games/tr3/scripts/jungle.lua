trx.events.on_game_start(function()
  trx.creatures.add_ally(trx.catalog.objects.monkey)
  -- Setup shoals
  trx.items[133].properties.range = { x = 10, y = 3, z = 22 }
end)
