trx.events.on_game_start(function()
  trx.objects.animating_13.properties.collidable = false
  trx.objects.animating_16.properties.collidable = false
  trx.items[89].properties.pickup_mode = trx.items.PickupMode.CROWBAR
  trx.items[177].properties.pickup_mode = trx.items.PickupMode.CROWBAR
end)
