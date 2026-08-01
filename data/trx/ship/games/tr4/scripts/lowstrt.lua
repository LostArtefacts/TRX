trx.events.on_game_start(function()
  trx.items[45].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW

  trx.objects.animating_13.properties.collidable = false
  trx.objects.animating_14.properties.collidable = false
  trx.objects.animating_16.properties.collidable = false
  trx.objects.switch_type_generic_1.properties.switch_mode =
    trx.items.SwitchMode.SHOVE
end)
