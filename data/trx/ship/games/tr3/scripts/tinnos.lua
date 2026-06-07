trx.events.before_item_setup(function(level)
  for _, item in
    ipairs(trx.items.find({
      object_id = trx.catalog.objects.animating_ext_1,
    }))
  do
    item.properties.kill_on_trigger = true
  end
end)
