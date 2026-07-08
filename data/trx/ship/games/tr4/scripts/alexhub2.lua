trx.events.before_item_setup(function(level)
  trx.objects.animating_14.properties.collidable = false
  trx.objects.animating_15.properties.collidable = false
  trx.objects.animating_16.properties.collidable = false
end)

trx.events.before_item_setup(function(level)
  trx.items[43].properties.crowbar = true
  trx.items[80].properties.crowbar = true
  trx.items[83].properties.crowbar = true
  trx.items[84].properties.crowbar = true
end)
