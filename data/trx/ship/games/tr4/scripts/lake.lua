trx.events.before_item_setup(function(level)
  trx.items[17].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[34].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[35].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[36].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[71].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[72].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW

  trx.objects.animating_16.properties.collidable = false
end)
