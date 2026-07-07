trx.events.before_item_setup(function(level)
  trx.items[4].properties.pickup_mode = trx.pickup.Mode.PLINTH_LOW
  trx.items[136].properties.pickup_mode = trx.pickup.Mode.PLINTH_LOW
end)
