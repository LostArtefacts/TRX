trx.events.before_item_setup(function(level)
  trx.items[12].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[135].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
end)
