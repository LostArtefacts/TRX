trx.events.before_item_setup(function(level)
  trx.items[3].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[135].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW

  trx.objects.animating_13.properties.collidable = false
  trx.objects.animating_14.properties.collidable = false
end)
