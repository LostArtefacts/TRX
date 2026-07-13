trx.events.before_item_setup(function(level)
  trx.items[59].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[93].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[105].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[121].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW

  trx.objects.animating_16.properties.collidable = false
end)
