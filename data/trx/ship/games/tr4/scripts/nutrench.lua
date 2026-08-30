trx.events.on_game_start(function()
  trx.objects.animating_16.properties.collidable = false
  trx.objects.switch_type_generic_1.properties.switch_mode =
    trx.items.SwitchMode.SHOVE
  trx.items[17].properties.pickup_mode = trx.items.PickupMode.CROWBAR
  trx.items[42].properties.collidable_when_done = false
end)
