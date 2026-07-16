trx.events.before_level_file(function(level)
  trx.creatures.add_ally(trx.catalog.objects.sthpac_mercenary)
end)

trx.events.before_item_setup(function(level)
  -- Setup shoals
  trx.items[114].properties.range = { x = 22, y = 6, z = 6 }
end)
