trx.events.on_game_start(function()
  trx.objects.animating_14.properties.collidable = false
  trx.objects.animating_15.properties.collidable = false
  trx.objects.animating_16.properties.collidable = false
  trx.items[42].properties.crowbar = true
  trx.items[79].properties.crowbar = true
  trx.items[82].properties.crowbar = true
  trx.items[83].properties.crowbar = true
  trx.items[36].properties.pickup_mode = trx.items.PickupMode.CROWBAR
  trx.objects.scaled_spikes.properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.ONE_SHOT
end)
