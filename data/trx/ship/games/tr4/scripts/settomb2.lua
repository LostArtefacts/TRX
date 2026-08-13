trx.events.on_game_start(function()
  trx.items[3].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[135].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW

  trx.objects.animating_13.properties.collidable = false
  trx.objects.animating_14.properties.collidable = false

  trx.objects.waterfall_1.properties.loop_sound = trx.items.WaterfallSound.SAND
  trx.objects.waterfall_1.properties.hide_when_inactive = true
end)
