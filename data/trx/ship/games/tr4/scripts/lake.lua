trx.events.before_item_setup(function(level)
  trx.items[16].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[33].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[34].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[35].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[70].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[71].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW

  trx.objects.animating_16.properties.collidable = false
end)
