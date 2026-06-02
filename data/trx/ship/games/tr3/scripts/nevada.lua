trx.events.before_item_setup(function(level)
  local props = trx.objects.cobra.properties
  props.alert_radius = 1.25
  props.attack_radius = 0.666667
  props.forget_radius = 2.5
end)
