trx.events.on_game_start(function()
  trx.creatures.add_ally(trx.catalog.objects.sthpac_mercenary)
  -- Setup shoals
  trx.items[114].properties.range = { x = 22, y = 6, z = 6 }
end)
