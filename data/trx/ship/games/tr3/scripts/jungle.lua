trx.events.before_level_file(function(level)
  trx.creatures.add_ally(trx.catalog.objects.monkey)
end)

trx.events.before_item_setup(function(level)
  -- Setup shoals
  trx.items[133].properties.range = { x = 10, y = 3, z = 22 }
end)
