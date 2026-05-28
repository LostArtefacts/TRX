trx.events.before_level_file(function(level)
  trx.creatures.add_ally(trx.catalog.objects.monkey)
end)

trx.events.before_item_setup(function(level)
  -- Setup shoals
  trx.items[66].properties.range = { x = 14, y = 6, z = 14 }
  trx.items[71].properties.range = { x = 14, y = 6, z = 14 }
end)
