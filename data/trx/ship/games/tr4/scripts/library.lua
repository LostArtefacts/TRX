trx.events.before_item_setup(function(level)
  trx.items[13].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[136].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
end)
