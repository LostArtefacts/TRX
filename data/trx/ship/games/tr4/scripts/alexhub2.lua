trx.events.on_game_start(function(level)
  trx.objects.animating_14.properties.collidable = false
  trx.objects.animating_15.properties.collidable = false
  trx.objects.animating_16.properties.collidable = false
  trx.items[42].properties.crowbar = true
  trx.items[79].properties.crowbar = true
  trx.items[82].properties.crowbar = true
  trx.items[83].properties.crowbar = true
  trx.items[36].properties.pickup_mode = trx.items.PickupMode.CROWBAR
end)
