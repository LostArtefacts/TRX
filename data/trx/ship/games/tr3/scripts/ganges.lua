trx.events.before_item_setup(function(level)
  local props = trx.objects[trx.catalog.objects.quad_bike].properties
  props.track_1 = 9
  props.track_2 = 12
  props.track_3 = 4
  props.track_4 = 12
end)
